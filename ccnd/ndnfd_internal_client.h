#ifdef WANT_NDNFD_INTERNAL_CLIENT_OPS

#define OP_SIGNATURE    0xD000
#define OP_STATS        0xD100
#define OP_LISTSTRATEGY 0xD200
#define OP_SETSTRATEGY  0xD300

#define NDNFD_DECL_REQ(op) \
int ndnfd_req_##op(struct ccnd_handle* h, const unsigned char* msg, size_t size, struct ccn_charbuf* reply_body);

NDNFD_DECL_REQ(signature);
NDNFD_DECL_REQ(newface);
NDNFD_DECL_REQ(destroyface);
NDNFD_DECL_REQ(stats);
NDNFD_DECL_REQ(liststrategy);
NDNFD_DECL_REQ(setstrategy);

#undef NDNFD_DECL_REQ
#undef WANT_NDNFD_INTERNAL_CLIENT_OPS
#endif

#ifdef WANT_NDNFD_ANSWER_REQ_CALLS

#define NDNFD_ANSWER_REQ(code,op) \
case code: \
  reply_body = ccn_charbuf_create(); \
  res = ndnfd_req_##op(ccnd, final_comp, final_size, reply_body); \
  break;

NDNFD_ANSWER_REQ(OP_SIGNATURE, signature);
NDNFD_ANSWER_REQ(OP_NEWFACE, newface);
NDNFD_ANSWER_REQ(OP_DESTROYFACE, destroyface);
NDNFD_ANSWER_REQ(OP_STATS, stats);
NDNFD_ANSWER_REQ(OP_LISTSTRATEGY, liststrategy);
NDNFD_ANSWER_REQ(OP_SETSTRATEGY, setstrategy);

#undef NDNFD_ANSWER_REQ
#undef WANT_NDNFD_ANSWER_REQ_CALLS
#endif

#ifdef WANT_NDNFD_URI_LISTEN

ccnd_uri_listen(ccnd, "ccnx:/ccnx/" CCND_ID_TEMPL "/ndnfd", &ccnd_answer_req, OP_SIGNATURE);
ccnd_uri_listen(ccnd, "ccnx:/ccnx/" CCND_ID_TEMPL "/ndnfd-stats", &ccnd_answer_req, OP_STATS);
ccnd_uri_listen(ccnd, "ccnx:/ccnx/" CCND_ID_TEMPL "/list-strategy", &ccnd_answer_req, OP_LISTSTRATEGY);
ccnd_uri_listen(ccnd, "ccnx:/ccnx/" CCND_ID_TEMPL "/set-strategy", &ccnd_answer_req, OP_SETSTRATEGY);

#undef WANT_NDNFD_URI_LISTEN
#endif

