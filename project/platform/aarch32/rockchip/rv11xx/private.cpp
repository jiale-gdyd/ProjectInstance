#include "private.hpp"

API_BEGIN_NAMESPACE(media)

RV1126::RV1126()
{

}

RV1126::~RV1126()
{

}

int RV1126::init()
{
    return getSys().init();
}

void RV1126::deinit()
{
    getSys().deinit();
}

API_END_NAMESPACE(media)
