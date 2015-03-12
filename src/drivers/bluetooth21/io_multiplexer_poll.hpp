#include <poll.h>

#include "std_algo.hpp"

#include "debug.hpp"
#include "host_protocol.hpp"
#include "io_multiplexer.hpp"
#include "unique_file.hpp"

namespace BT
{

template <typename Device>
void
perform_poll_io(Device & d, MultiPlexer & mp, int poll_timeout_ms)
{
	poll_notify_mask_t readable, writeable;
	if (empty(mp.xt.device_buffer))
	{
		lock_guard guard(mp.mutex_xt);
		writeable = fill_device_buffer(mp.protocol_tag, mp.xt);
	}

	pollfd p;
	p.fd = fileno(d);
	p.events = POLLIN;
	if (not empty(mp.xt.device_buffer)) { p.events |= POLLOUT; }

	int r = poll(&p, 1, poll_timeout_ms);
	if (r == -1)
		perror("perform_poll_io / poll");
	else if (r == 1)
	{
		/*
		 * Input comes before output as we have
		 * to check what the chip is ready to receive and
		 * to inform the chip about what we are ready to receive.
		 *
		 * This property is not fully used now.
		 */
		if (p.revents & POLLIN)
		{
			lock_guard guard(mp.mutex_rx);
			readable = process_serial_input(mp.protocol_tag, d, mp.rx);
		}
		if (p.revents & POLLOUT)
			writeable |= process_serial_output(mp.protocol_tag, d, mp.xt);
	}

	if (not (empty(readable) and empty(writeable)))
		poll_notify_by_masks(mp.poll_waiters, readable, writeable);
}

}
// end of namespace BT
