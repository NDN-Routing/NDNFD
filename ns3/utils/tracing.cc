#include "tracing.h"
#include <ns3/config.h>
#include <ns3/simulator.h>
namespace ndnfd {

Tracer::Tracer(const std::string& filename)
    : total_mcast_send_(0), total_mcast_recv_(0), total_unicast_send_(0), recent_mcast_send_(0), recent_mcast_recv_(0), recent_unicast_send_(0) {
  this->outfile_ = fopen(filename.c_str(), "w");
  assert(this->outfile_ != nullptr);
  this->Connect();
  this->PrintHeader();
  this->PeriodicPrinter();
}

Tracer::~Tracer(void) {
  fclose(this->outfile_);
}

void Tracer::PrintHeader(void) {
  fprintf(this->outfile_, "TIME\tRECENT-MCAST-SEND\tRECENT-MCAST-RECV\tRECENT-UNICAST-SEND\tTOTAL-MCAST-SEND\tTOTAL-MCAST-RECV\tTOTAL-UNICAST-SEND\n");
}

void Tracer::Print(void) {
  fprintf(this->outfile_, "%" PRIuMAX "\t%" PRIuMAX "\t%" PRIuMAX "\t%" PRIuMAX "\t%" PRIuMAX "\t%" PRIuMAX "\t%" PRIuMAX "\n", static_cast<uintmax_t>(ns3::Now().GetSeconds()), this->recent_mcast_send_, this->recent_mcast_recv_, this->recent_unicast_send_, this->total_mcast_send_, this->total_mcast_recv_, this->total_unicast_send_);
}

void Tracer::PeriodicPrinter(void) {
  this->Print();
  this->recent_mcast_send_ = this->recent_mcast_recv_ = this->recent_unicast_send_ = 0;
  ns3::Simulator::Schedule(ns3::Seconds(1.0), &Tracer::PeriodicPrinter, this);
}

void Tracer::Connect(void) {
  ns3::Config::ConnectWithoutContext("/NodeList/*/$ndnfd::L3Protocol/InterestMcastSend", ns3::MakeCallback(&Tracer::InterestMcastSend, this));
  ns3::Config::ConnectWithoutContext("/NodeList/*/$ndnfd::L3Protocol/InterestMcastRecv", ns3::MakeCallback(&Tracer::InterestMcastRecv, this));
  ns3::Config::ConnectWithoutContext("/NodeList/*/$ndnfd::L3Protocol/InterestUnicastSend", ns3::MakeCallback(&Tracer::InterestUnicastSend, this));
}

void Tracer::InterestMcastSend(ns3::Ptr<L3Protocol> l3, const Name* name) {
  ++this->recent_mcast_send_;
  ++this->total_mcast_send_;
}

void Tracer::InterestMcastRecv(ns3::Ptr<L3Protocol> l3, const Name* name) {
  ++this->recent_mcast_recv_;
  ++this->total_mcast_recv_;
}

void Tracer::InterestUnicastSend(ns3::Ptr<L3Protocol> l3, const Name* name) {
  ++this->recent_unicast_send_;
  ++this->total_unicast_send_;
}


};//namespace ndnfd
