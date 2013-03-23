#ifdef WANT_NDNFD_INTERNAL_CLIENT_OPS

#define OP_SIGNATURE 0xD000

#define NDNFD_DECL_REQ(op) \
int ndnfd_req_##op(struct ccnd_handle* h, const unsigned char* msg, size_t size, struct ccn_charbuf* reply_body);
NDNFD_DECL_REQ(signature);
NDNFD_DECL_REQ(newface);
NDNFD_DECL_REQ(destroyface);

#undef NDNFD_DECL_REQ
#undef WANT_NDNFD_INTERNAL_CLIENT_OPS
#endif

#ifdef WANT_NDNFD_ANSWER_REQ_CALLS

case OP_SIGNATURE://a signature that detect the router is NDNFD
  reply_body = ccn_charbuf_create();
  res = ndnfd_req_signature(ccnd, final_comp, final_size, reply_body);
  break;
case OP_NEWFACE://Face Management protocol, newface command
  reply_body = ccn_charbuf_create();
  res = ndnfd_req_newface(ccnd, final_comp, final_size, reply_body);
  break;
case OP_DESTROYFACE://Face Management protocol, destroyface command
  reply_body = ccn_charbuf_create();
  res = ndnfd_req_destroyface(ccnd, final_comp, final_size, reply_body);
  break;

#undef WANT_NDNFD_ANSWER_REQ_CALLS
#endif

#ifdef WANT_NDNFD_URI_LISTEN

ccnd_uri_listen(ccnd, "ccnx:/ccnx/" CCND_ID_TEMPL "/ndnfd", &ccnd_answer_req, OP_SIGNATURE);

#undef WANT_NDNFD_URI_LISTEN
#endif

