#define ANY_LOG_IMPLEMENT
#define ANY_LOG_MODULE "test"
#include "any_log.h"

int main()
{
	log_trace("Hello");
	log_debug("Hello");
	log_info("Hello");
	log_warn("Hello");
	log_error("Hello");
	log_panic("Hello");

	return 0;
}
