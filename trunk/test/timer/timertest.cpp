
#pragma warning (disable : 4786)

#include <stdio.h>
#include <assert.h>

#include "timer.h"

// TOLERANCE is in units of ms
#define TOLERANCE 250

class timer_client : public TimerClient
{
public:

	virtual void on_timer(const Timer::TimerID &id);
};

timer_client client;
omni_mutex mutex;
Timer timer;
std::set<Timer::TimerID> expected;
bool exit_flag = false;
bool sanity_check = true;

void check_complete(void)
{
	if (expected.size() == 0 && exit_flag)
	{
		printf("ok: got all outstanding timers. exiting.\n");

		exit(0);
	}
}

void stop(void)
{
	exit_flag = true;

	printf("stopping\n");

	check_complete();

	printf("waiting for %d outstanding timers\n", expected.size());
}

void* add_timers(void* arg)
{
	while(true)
	{
		omni_thread::sleep(0, (rand() % 5000) * 10000);

		{
			omni_mutex_lock l(mutex);

			if (exit_flag)
				return NULL;

			if (expected.size() > 200)
			{
				continue;
			}

			int r = rand() % 5000;

			printf("added timer %d ms later\n", r);
			expected.insert(timer.add(r, &client));
		}
	}

	return NULL;
}

void* remove_timers(void* arg)
{
	while(true)
	{
		omni_thread::sleep(0, (rand() % 5000) * 100000);

		{
			omni_mutex_lock l(mutex);

			if (exit_flag)
				return NULL;

			if (expected.size())
			{
				int r = rand() % expected.size();

				int i = 0;
				std::set<Timer::TimerID>::iterator it;
				for (it	= expected.begin(); i < r; ++i, ++it);

				if (it != expected.end())
				{
					printf("removing timer\n");

					if (timer.remove(*it))
						expected.erase(it);
				}
			}
		}
	}

	return NULL;
}

void timer_client::on_timer(const Timer::TimerID &id)
{
	omni_mutex_lock l(mutex);

	std::set<Timer::TimerID>::iterator i = expected.find(id);

	if (i == expected.end())
	{
		printf("error: unexpected timer expired\n");

		stop();
	}
	else
	{
		expected.erase(id);

		unsigned long abs_sec;
		unsigned long abs_nsec;

		omni_thread::get_time(&abs_sec, &abs_nsec);

		if (abs_sec < id.m_abs_sec)
		{
			printf("error: timer not yet expired\n");

			stop();
		}

		unsigned long d = (abs_sec - id.m_abs_sec) * 1000;

		// we need to calculate using unsigned longs
		if (abs_nsec > id.m_abs_nsec)
			d += (abs_nsec - id.m_abs_nsec) / 1000000;
		else
			d -= (id.m_abs_nsec - abs_nsec) / 1000000;

		if (d <= TOLERANCE)
			printf("ok: timer expired. deviation: %lu ms\n", d);
		else
		{
			printf("error: timer expired. deviation: %lu ms\n", d);

			stop();
		}

		check_complete();
	}

	if (sanity_check && expected.size() == 0)
	{
		printf("\nok. sanity checks completed successfully.\n");
		printf("got exactly the expected timers with acceptable deviation.\n");
		printf("now starting random tests.\n\n");

		sanity_check = false;

		omni_thread::create(add_timers);
		omni_thread::create(remove_timers);
	}
}

int main(int argc, char* argv[])
{
	printf("Caution: these tests may fail with a positive deviation > %d ms.\n", TOLERANCE); 
	printf("This does not indicate an error\n");
	printf("hit return to stop\n");

	// add a timer in 1000 ms
	expected.insert(timer.add(1000, &client));

	// add and remove a timer in 500 ms (before the first existing timer)
	Timer::TimerID id = timer.add(500, &client);
	timer.remove(id);

	// add two timers in 500 ms
	expected.insert(timer.add(500, &client));
	expected.insert(timer.add(500, &client));

	// add a timer in 2000 ms
	expected.insert(timer.add(2000, &client));

	// add and remove a timer in 3000 ms (as last)
	id = timer.add(3000, &client);
	timer.remove(id);

	// remove a timer twice
	timer.remove(id);

	// add two timers in 4000 ms
	expected.insert(timer.add(4000, &client));
	expected.insert(timer.add(4000, &client));

	// add a timer in 3000 ms
	expected.insert(timer.add(3000, &client));

	timer.start();

	if (getchar())
	{
		stop();

		omni_thread::sleep(-1, 0);
	}

	return 0;
}
