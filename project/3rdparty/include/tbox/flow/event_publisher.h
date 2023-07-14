#ifndef TBOX_FLOW_EVENT_PUBLISHER_H
#define TBOX_FLOW_EVENT_PUBLISHER_H

#include "event.h"

namespace tbox {
namespace flow {

class EventSubscriber;

class EventPublisher {
  public:
    virtual void subscribe(EventSubscriber *subscriber) = 0;
    virtual void unsubscribe(EventSubscriber *subscriber) = 0;
    virtual void publish(Event event) = 0;

  protected:
    virtual ~EventPublisher() { }
};

}
}

#endif
