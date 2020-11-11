#include "FairLossLink.hpp"
#include "PLMessage.hpp"
#include "data-structures.hpp"

/* Need to add no duplication property */
class PerfectLink {
public:
  FairLossLink FLL; /* For layering */
  SharedQueue<AppMessage> incoming;
  SharedQueue<AppMessage> outgoing;

  /* Map of already delivered messages to layer above. Can be std::map because
   * it is handled by a single thread */
  std::string identifier = "[PL]:"; // For debugging
  PerfectLink() {}
  ~PerfectLink() {}
};
/* Interface */
void PL_init(PerfectLink *link, unsigned long host_id,
             std::vector<Parser::Host> hosts, std::thread *threads,
             uint *thread_no); /* Bad layering ?*/

void PL_send(PerfectLink *link, Message msg, unsigned long host_id);
Message PL_recv(PerfectLink *link);

/* Internal functions */
void retry(PerfectLink *link);
void populate_delivery(PerfectLink *link)

    /* Definitions */