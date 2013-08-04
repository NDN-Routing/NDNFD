#include "message_counter.h"
#include <ns3/config.h>
#include <ns3/simulator.h>
#include "message/interest.h"
#include "message/contentobject.h"
#include "message/nack.h"
namespace ndnfd {

MessageCounter::MessageCounter(const std::string& filename)
    : SMI_(0), SMC_(0), SMN_(0), SUI_(0), SUC_(0), SUN_(0), RMI_(0), RMC_(0), RMN_(0), RUI_(0), RUC_(0), RUN_(0) {
  this->outfile_ = fopen(filename.c_str(), "w");
  assert(this->outfile_ != nullptr);
}

MessageCounter::~MessageCounter(void) {
  fclose(this->outfile_);
}

void MessageCounter::PrintHeader(void) {
  fprintf(this->outfile_, "TIME\tSMI\tSMC\tSMN\tSUI\tSUC\tSUN\tRMI\tRMC\tRMN\tRUI\tRUC\tRUN\n");
}

void MessageCounter::Print(void) {
  fprintf(this->outfile_, "%" PRIuMAX "\t%" PRIuMAX "\t%" PRIuMAX "\t%" PRIuMAX "\t%" PRIuMAX "\t%" PRIuMAX "\t%" PRIuMAX "\t%" PRIuMAX "\t%" PRIuMAX "\t%" PRIuMAX "\t%" PRIuMAX "\t%" PRIuMAX "\t%" PRIuMAX "\n", static_cast<uintmax_t>(ns3::Now().GetSeconds()), this->SMI_, this->SMC_, this->SMN_, this->SUI_, this->SUC_, this->SUN_, this->RMI_, this->RMC_, this->RMN_, this->RUI_, this->RUC_, this->RUN_);
}

void MessageCounter::PeriodicPrinter(void) {
  this->Print();
  ns3::Simulator::Schedule(ns3::Seconds(1.0), &MessageCounter::PeriodicPrinter, this);
}

void MessageCounter::ConnectNode(ns3::Ptr<ns3::Node> node) {
  assert(node != nullptr);
  char node_selector[16];
  snprintf(node_selector, sizeof(node_selector), "%" PRIu32 "", node->GetId());
  this->Connect(node_selector);
}

void MessageCounter::Connect(const std::string& node_selector) {
  ns3::Config::ConnectWithoutContext("/NodeList/" + node_selector + "/$ndnfd::L3Protocol/MessageSend", ns3::MakeCallback(&MessageCounter::OnSend, this));
  ns3::Config::ConnectWithoutContext("/NodeList/" + node_selector + "/$ndnfd::L3Protocol/MessageRecv", ns3::MakeCallback(&MessageCounter::OnRecv, this));

  this->PrintHeader();
  this->PeriodicPrinter();
}

void MessageCounter::OnSend(ns3::Ptr<L3Protocol> l3, MessageType t, bool is_mcast) {
  if (is_mcast) {
    if (t == InterestMessage::kType) { ++this->SMI_; }
    else if (t == ContentObjectMessage::kType) { ++this->SMC_; }
    else if (t == NackMessage::kType) { ++this->SMN_; }
  } else {
    if (t == InterestMessage::kType) { ++this->SUI_; }
    else if (t == ContentObjectMessage::kType) { ++this->SUC_; }
    else if (t == NackMessage::kType) { ++this->SUN_; }
  }
}

void MessageCounter::OnRecv(ns3::Ptr<L3Protocol> l3, MessageType t, bool is_mcast) {
  if (is_mcast) {
    if (t == InterestMessage::kType) { ++this->RMI_; }
    else if (t == ContentObjectMessage::kType) { ++this->RMC_; }
    else if (t == NackMessage::kType) { ++this->RMN_; }
  } else {
    if (t == InterestMessage::kType) { ++this->RUI_; }
    else if (t == ContentObjectMessage::kType) { ++this->RUC_; }
    else if (t == NackMessage::kType) { ++this->RUN_; }
  }
}

};//namespace ndnfd
