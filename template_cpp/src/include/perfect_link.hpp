#include "fairlosslink.hpp"
#include "message.hpp"
#include "shared-data.h"

void perfectSend(Message msg, size_t host_id) {}

class PerfectLink {
public:
  FairLossLink FLL;              /* For layering */
  SharedQueue<Message> delivery; /* Output queue */
  /* collection of un-acked messages. Needs to be a shared DS */
  /* Map of already delivered messages to layer above. Can be std::map because
   * it is handled by a single thread */
  std::map<unsigned long, struct sockaddr_in>
      others; /* Bad layering. Also read only */

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