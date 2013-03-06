#ifdef WANT_STRATEGY_CALLOUT

static void
strategy_callout(struct ccnd_handle *h,
                 struct interest_entry *ie,
                 enum ccn_strategy_op op)
{
    struct pit_face_item *x = NULL;
    struct pit_face_item *p = NULL;
    struct nameprefix_entry *npe = NULL;
    struct ccn_indexbuf *tap = NULL;
    unsigned best = CCN_NOFACEID;
    unsigned randlow, randrange;
    unsigned nleft;
    unsigned amt;
    int usec;
    int usefirst;
    
    switch (op) {
        case CCNST_NOP:
            break;
        case CCNST_FIRST:
            
            npe = get_fib_npe(h, ie);
            if (npe != NULL)
                tap = npe->tap;
            npe = ie->ll.npe;
            best = npe->src;
            if (best == CCN_NOFACEID)
                best = npe->src = npe->osrc;
            /* Find our downstream; right now there should be just one. */
            for (x = ie->pfl; x != NULL; x = x->next)
                if ((x->pfi_flags & CCND_PFI_DNSTREAM) != 0)
                    break;
            if (x == NULL || (x->pfi_flags & CCND_PFI_PENDING) == 0) {
                ccnd_debug_ccnb(h, __LINE__, "canthappen", NULL,
                                ie->interest_msg, ie->size);
                break;
            }
            if (best == CCN_NOFACEID || npe->usec > 150000) {
                usefirst = 1;
                randlow = 4000;
                randrange = 75000;
            }
            else {
                usefirst = 0;
                randlow = npe->usec;
                randrange = (randlow + 1) / 2;
            }
            nleft = 0;
            for (p = ie->pfl; p!= NULL; p = p->next) {
                if ((p->pfi_flags & CCND_PFI_UPSTREAM) != 0) {
                    if (p->faceid == best) {
                        p = send_interest(h, ie, x, p);
                        strategy_settimer(h, ie, npe->usec, CCNST_TIMER);
                    }
                    else if (ccn_indexbuf_member(tap, p->faceid) >= 0)
                        p = send_interest(h, ie, x, p);
                    else if (usefirst) {
                        usefirst = 0;
                        pfi_set_expiry_from_micros(h, ie, p, 0);
                    }
                    else if (p->faceid == npe->osrc)
                        pfi_set_expiry_from_micros(h, ie, p, randlow);
                    else {
                        /* Want to preserve the order of the rest */
                        nleft++;
                        p->pfi_flags |= CCND_PFI_SENDUPST;
                    }
                }
            }
            if (nleft > 0) {
                /* Send remainder in order, with randomized timing */
                amt = (2 * randrange + nleft - 1) / nleft;
                if (amt == 0) amt = 1; /* paranoia - should never happen */
                usec = randlow;
                for (p = ie->pfl; p!= NULL; p = p->next) {
                    if ((p->pfi_flags & CCND_PFI_SENDUPST) != 0) {
                        pfi_set_expiry_from_micros(h, ie, p, usec);
                        usec += nrand48(h->seed) % amt;
                    }
                }
            }
            break;
        case CCNST_TIMER:
            /*
             * Our best choice has not responded in time.
             * Increase the predicted response.
             */
            adjust_predicted_response(h, ie, 1);
            break;
        case CCNST_SATISFIED:
            break;
        case CCNST_TIMEOUT:
            break;
    }
}

#undef WANT_STRATEGY_CALLOUT
#endif

#ifdef WANT_DO_PROPAGATE

/**
 * Execute the next timed action on a propagating interest.
 */
static int
do_propagate(struct ccn_schedule *sched,
             void *clienth,
             struct ccn_scheduled_event *ev,
             int flags)
{
    struct ccnd_handle *h = clienth;
    struct interest_entry *ie = ev->evdata;
    struct face *face = NULL;
    struct pit_face_item *p = NULL;
    struct pit_face_item *next = NULL;
    struct pit_face_item *d[3] = { NULL, NULL, NULL };
    ccn_wrappedtime now;
    int next_delay;
    int i;
    int n;
    int pending;
    int upstreams;
    unsigned life;
    unsigned mn;
    unsigned rem;
    
    if (ie->ev == ev)
        ie->ev = NULL;
    if (flags & CCN_SCHEDULE_CANCEL)
        return(0);
    now = h->wtnow;  /* capture our reference */
    mn = 600 * WTHZ; /* keep track of when we should wake up again */
    pending = 0;
    n = 0;
    for (p = ie->pfl; p != NULL; p = next) {
        next = p->next;
        if ((p->pfi_flags & CCND_PFI_DNSTREAM) != 0) {
            if (wt_compare(p->expiry, now) <= 0) {
                if (h->debug & 2)
                    ccnd_debug_ccnb(h, __LINE__, "interest_expiry",
                                    face_from_faceid(h, p->faceid),
                                    ie->interest_msg, ie->size);
                pfi_destroy(h, ie, p);
                continue;
            }
            if ((p->pfi_flags & CCND_PFI_PENDING) == 0)
                continue;
            rem = p->expiry - now;
            if (rem < mn)
                mn = rem;
            pending++;
            /* If this downstream will expire soon, don't use it */
            life = p->expiry - p->renewed;
            if (rem * 8 <= life)
                continue;
            /* keep track of the 2 longest-lasting downstreams */
            for (i = n; i > 0 && wt_compare(d[i-1]->expiry, p->expiry) < 0; i--)
                d[i] = d[i-1];
            d[i] = p;
            if (n < 2)
                n++;
        }
    }
    /* Send the interests out */
    upstreams = 0; /* Count unexpired upstreams */
    for (p = ie->pfl; p != NULL; p = next) {
        next = p->next;
        if ((p->pfi_flags & CCND_PFI_UPSTREAM) == 0)
            continue;
        face = face_from_faceid(h, p->faceid);
        if (face == NULL || (face->flags & CCN_FACE_NOSEND) != 0) {
            pfi_destroy(h, ie, p);
            continue;
        }
        if ((face->flags & CCN_FACE_DC) != 0 &&
            (p->pfi_flags & CCND_PFI_DCFACE) == 0) {
            /* Add 60 ms extra delay before sending to a DC face */
            p->expiry += (60 * WTHZ + 999) / 1000;
            p->pfi_flags |= CCND_PFI_DCFACE;
        }
        if (wt_compare(now + 1, p->expiry) < 0) {
            /* Not expired yet */
            rem = p->expiry - now;
            if (rem < mn)
                mn = rem;
            upstreams++;
            continue;
        }
        for (i = 0; i < n; i++)
            if (d[i]->faceid != p->faceid)
                break;
        if (i < n) {
            p = send_interest(h, ie, d[i], p);
            upstreams++;
            rem = p->expiry - now;
            if (rem < mn)
                mn = rem;
        }
        else {
            /* Upstream expired, but we have nothing to feed it. */
            p->pfi_flags |= CCND_PFI_UPHUNGRY;
        }
    }
    if (pending == 0 && upstreams == 0) {
        strategy_callout(h, ie, CCNST_TIMEOUT);
        consume_interest(h, ie);
        return(0);
    }
    /* Determine when we need to run again */
    if (mn == 0) abort();
    next_delay = mn * (1000000 / WTHZ);
    ev->evint = h->wtnow + mn;
    ie->ev = ev;
    return(next_delay);
}

#undef WANT_DO_PROPAGATE
#endif

#ifdef WANT_MATCH_INTERESTS

/**
 * Find and consume interests that match given content.
 *
 * Schedules the sending of the content.
 * If face is not NULL, pay attention only to interests from that face.
 * It is allowed to pass NULL for pc, but if you have a (valid) one it
 * will avoid a re-parse.
 * For new content, from_face is the source; for old content, from_face is NULL.
 * @returns number of matches, or -1 if the new content should be dropped.
 */
static int
match_interests(struct ccnd_handle *h, struct content_entry *content,
                           struct ccn_parsed_ContentObject *pc,
                           struct face *face, struct face *from_face)
{
    int n_matched = 0;
    int new_matches;
    int ci;
    int cm = 0;
    unsigned c0 = content->comps[0];
    const unsigned char *key = content->key + c0;
    struct nameprefix_entry *npe = NULL;
    for (ci = content->ncomps - 1; ci >= 0; ci--) {
        int size = content->comps[ci] - c0;
        npe = hashtb_lookup(h->nameprefix_tab, key, size);
        if (npe != NULL)
            break;
    }
    for (; npe != NULL; npe = npe->parent, ci--) {
        if (npe->fgen != h->forward_to_gen)
            update_forward_to(h, npe);
        if (from_face != NULL && (npe->flags & CCN_FORW_LOCAL) != 0 &&
            (from_face->flags & CCN_FACE_GG) == 0)
            return(-1);
        new_matches = consume_matching_interests(h, npe, content, pc, face);
        if (from_face != NULL && (new_matches != 0 || ci + 1 == cm))
            note_content_from(h, npe, from_face->faceid, ci);
        if (new_matches != 0) {
            cm = ci; /* update stats for this prefix and one shorter */
            n_matched += new_matches;
        }
    }
    return(n_matched);
}

#undef WANT_MATCH_INTERESTS
#endif

