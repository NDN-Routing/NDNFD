#include "nack.h"
extern "C" {
#include <ccn/coding.h>
int ccn_parse_optional_tagged_UDATA(struct ccn_buf_decoder* d, enum ccn_dtag dtag);
}
namespace ndnfd {

bool NackCode_valid(NackCode code) {
  switch (code) {
    case NackCode::kDuplicate:
    case NackCode::kCongestion:
    case NackCode::kNoData:
      return true;
  }
  return false;
}

std::string NackCode_string(NackCode code) {
  switch (code) {
    case NackCode::kDuplicate:
      return "Duplicate";
    case NackCode::kCongestion:
      return "Congestion";
    case NackCode::kNoData:
      return "NoData";
  }
  return "";
}

const MessageType NackMessage::kType;

NackMessage::NackMessage(const uint8_t* msg, size_t length) : CcnbMessage(msg, length) {}

Ptr<NackMessage> NackMessage::Parse(const uint8_t* msg, size_t length) {
  struct ccn_buf_decoder decoder;
  struct ccn_buf_decoder* d = ccn_buf_decoder_start(&decoder, msg, length);
  if (0 == ccn_buf_match_dtag(d, CCN_DTAG_StatusResponse)) return nullptr;
  ccn_buf_advance(d);// <StatusResponse>

  int code_n = ccn_parse_optional_tagged_nonNegativeInteger(d, CCN_DTAG_StatusCode);
  NackCode code = static_cast<NackCode>(code_n);
  if (!NackCode_valid(code)) return nullptr;
  
  ccn_parse_optional_tagged_UDATA(d, CCN_DTAG_StatusText);
  if (d->decoder.state < 0) return nullptr;
  
  if (0 == ccn_buf_match_dtag(d, CCN_DTAG_Interest)) return nullptr;
  const uint8_t* interest_msg = msg + d->decoder.token_index;
  size_t interest_length = length - d->decoder.token_index - 1;
  Ptr<InterestMessage> interest = InterestMessage::Parse(interest_msg, interest_length);
  if (interest == nullptr) return nullptr;
  
  if (interest_msg[interest_length] != '\0') return nullptr;// </StatusResponse>

  Ptr<NackMessage> nack = new NackMessage(msg, length);
  nack->code_ = code;
  nack->interest_ = interest;
  return nack;
}

Ptr<Buffer> Create(NackCode code, Ptr<InterestMessage> interest) {
  assert(NackCode_valid(code));
  assert(interest != nullptr);
  
  ccn_charbuf* b = ccn_charbuf_create();
  ccn_charbuf_append_tt(b, CCN_DTAG_StatusResponse, CCN_DTAG);
  ccnb_tagged_putf(b, CCN_DTAG_StatusCode, "%d", static_cast<int>(code));
  ccn_charbuf_append(b, interest->msg(), interest->length());
  ccn_charbuf_append_closer(b);
  Ptr<Buffer> buf = new Buffer(0);
  buf->Swap(b);
  ccn_charbuf_destroy(&b);
  return buf;
}

};//namespace ndnfd
