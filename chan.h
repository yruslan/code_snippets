#ifndef _CHAN_H_INCLUDED_2016_05_30
#define _CHAN_H_INCLUDED_2016_05_30

/* Minimalistic implementation of multithreaded channels
*
* This channel implementation has following features
*
* Assumptions
* - C++11 required
*
* Drawbacks
*
* Anyone can use it freely for any purpose. There is
* absolutely no guarantee it works or fits a particular purpose (see below).
*
* Copyright (C) by Ruslan Yushchenko (yruslan@gmail.com)
*
* This is free and unencumbered software released into the public domain.
*
* Anyone is free to copy, modify, publish, use, compile, sell, or
* distribute this software, either in source code form or as a compiled
* binary, for any purpose, commercial or non-commercial, and by any
* means.
*
* In jurisdictions that recognize copyright laws, the author or authors
* of this software dedicate any and all copyright interest in the
* software to the public domain. We make this dedication for the benefit
* of the public at large and to the detriment of our heirs and
* successors. We intend this dedication to be an overt act of
* relinquishment in perpetuity of all present and future rights to this
* software under copyright law.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*
* For more information, please refer to <http://unlicense.org/>
*/

#include <deque>
#include <mutex> 
#include <condition_variable>
#include <list>
#include <atomic>
#include <thread>

namespace mtools
{
	namespace exceptions 
	{
		class bad_all_writers : public std::logic_error
		{
		public:
			bad_all_writers()
				: std::logic_error("Channel has only writers - will block forever.")
			{}
		};

		class bad_all_readers : public std::logic_error
		{
		public:
			bad_all_readers()
				: std::logic_error("Channel has only readers - will block forever.")
			{}
		};

		class bad_closed : public std::logic_error
		{
		public:
			bad_closed()
				: std::logic_error("Attempt to write to closed channel")
			{}
		};
	}

	class mutex
	{
	public:
		mutex() : mtx(raw_mtx, std::defer_lock) {};
		void lock() { mtx.lock(); };
		bool try_lock() { return mtx.try_lock(); };
		void unlock() { mtx.unlock(); };
	private:
		std::unique_lock<std::mutex> mtx;
		std::mutex raw_mtx;
		friend class condvar;
	};

	class mutex_guard
	{
	public:
		mutex_guard(mutex &mtx_) : mtx(mtx_) { mtx.lock(); };
		~mutex_guard() { mtx.unlock(); };

		mutex_guard(const mutex_guard &m) = delete;
		mutex_guard(mutex_guard &&m) = delete;
		mutex_guard &operator=(const mutex_guard &m) = delete;
		mutex_guard &operator=(mutex_guard &&m) = delete;
	private:
		mutex &mtx;
	};

	class condvar
	{
	public:
		void wait(mutex &mtx)
		{
			cv.wait(mtx.mtx);
		};
		void notify_one() { cv.notify_one(); };
		void notify_all() { cv.notify_all(); };
	private:
		std::condition_variable cv;
	};

	class sem
	{
	public:
		explicit sem(int cnt) : n(cnt) {};
		void p()
		{
			mutex_guard grd(m);			
			while (n <= 0)
				cv.wait(m);
		};
		void v()
		{
			mutex_guard grd(m);
			n++;
			cv.notify_one();
		};
	private:
		int n;
		condvar cv;
		mutex m;
	};

	enum class tchan_state
	{
		CLOSED = 0, OPEN
	};

	namespace details
	{
		// Implementation of bounded queue
		template<typename T>
		class queue
		{
		public:
			explicit queue(int cap) { };
			void push(const T& v) { q.push_back(v); };
			void push(T&& v) { q.push_back(std::move(v)); };
			void pop(T &v) { v = std::move(q.front()); q.pop_front(); };
			size_t size() { return q.size(); };
		private:
			std::deque<T> q;
		};

		template<typename T>
		class chan_details
		{
		public:
			explicit chan_details(int capacity);
			tchan_state get_state() const;
			int get_capacity() const;

			const mutex *get_mutex() const;
			const condvar *get_condvar() const;

			int cap;
			int readers, writers;
			std::atomic<int> refs;
			tchan_state state;
			mutex mtx;
			condvar crd, cwr;
			details::queue<T> q;
			std::list<sem *> waiters;
			T* sync;
		};

		template <typename T>
		chan_details<T>::chan_details(int capacity)
			: cap(capacity), readers(0), writers(0), refs(1), state(tchan_state::OPEN), q(cap), sync(nullptr)
		{
		}

		template <typename T>
		tchan_state chan_details<T>::get_state() const
		{
			return state;
		}

		template <typename T>
		int chan_details<T>::get_capacity() const
		{
			return cap;
		}

		template <typename T>
		const mutex* chan_details<T>::get_mutex() const
		{
			return &mtx;
		}

		template <typename T>
		const condvar* chan_details<T>::get_condvar() const
		{
			return &crd;
		}
	}

	class ichannel
	{
	public:
		virtual ~ichannel() {};

		virtual tchan_state get_state() = 0;
		virtual int get_buf_capacity() = 0;
		virtual bool is_same(ichannel *rhs) = 0;

	protected:
		virtual int get_buf_size() = 0;
		virtual mutex *get_mutex() = 0;
		virtual condvar *get_rcond() = 0;
		virtual void add_waiter(sem *cv) = 0;
		virtual void del_waiter(sem *cv) = 0;
		virtual void *get_d() = 0;
		friend ichannel *select(std::initializer_list<ichannel *> lst);
		template <typename U1> friend class chan;
	};

	template<typename T>
	class chan : public ichannel
	{
	public:
		explicit chan(int capacity = 0);
		chan(const chan<T> &ch);
		~chan();

		void make(int capacity);
		void close();
		void send(const T&v);
		void send(T &&v);
		bool try_send(const T&v);
		void recv(T&v);
		bool try_recv(T&v);

		tchan_state get_state() override;
		int get_buf_size() override;
		int get_buf_capacity() override;
		bool is_same(ichannel *rhs) override;

	protected:
		mutex *get_mutex() override;
		condvar *get_rcond() override;
		void add_waiter(sem *cv)override;
		void del_waiter(sem *cv)override;
		void *get_d() override;

	private:
		void destroy();
		void notify_readers();
		details::chan_details<T> *d;
	};

	inline ichannel *select(std::initializer_list<ichannel *> lst)
	{
		sem sm(0);
		mutex *chan_mtx;
		ichannel *pChan = nullptr;
		int chanCnt = static_cast<int>(lst.size());
		if (chanCnt == 0)
			return pChan;		
		for (int i = 0; i < chanCnt; i++)
		{
			pChan = *(lst.begin() + i);
			chan_mtx = pChan->get_mutex();
			chan_mtx->lock();
			if (pChan->get_buf_size()>0 || pChan->get_state() == tchan_state::CLOSED)
			{
				chan_mtx->unlock();
				for (int j = 0; j < i; j++)
				{
					ichannel *p = *(lst.begin() + j);
					p->del_waiter(&sm);
				}
				return pChan;
			}
			pChan->add_waiter(&sm);
			chan_mtx->unlock();
		}
		while (true)
		{
			// Rechecking all channels
			for (int i = 0; i < chanCnt; i++)
			{
				pChan = *(lst.begin() + i);
				chan_mtx = pChan->get_mutex();
				chan_mtx->lock();
				if (pChan->get_buf_size()>0 || pChan->get_state() == tchan_state::CLOSED)
				{
					chan_mtx->unlock();
					for (int j = 0; j < chanCnt; j++)
					{
						ichannel *p = *(lst.begin() + j);
						p->del_waiter(&sm);
					}
					return pChan;
				}
				chan_mtx->unlock(); 
				// simulate race condition (in debugger)
				//std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			sm.p();
		}
	}

	// -------------- channel implementation ------------

	template <typename T>
	chan<T>::chan(int capacity)
	{
		d = new details::chan_details<T>(capacity);
	}

	template <typename T>
	chan<T>::chan(const chan<T>& ch)
	{
		d = ch.d;
		++d->refs;
	}

	template <typename T>
	chan<T>::~chan()
	{
		destroy();
	}

	template <typename T>
	void chan<T>::make(int capacity)
	{
		destroy();
		d = new details::chan_details<T>(capacity);
	}

	template <typename T>
	void chan<T>::close()
	{
		mutex_guard grd(d->mtx);
		d->state = tchan_state::CLOSED;
		for (auto &w : d->waiters)
		{
			w->v();
		}
		d->crd.notify_all();
		d->cwr.notify_all();
	}

	template <typename T>
	void chan<T>::send(const T& v)
	{
		if (d->get_state() == tchan_state::CLOSED)
			throw exceptions::bad_closed();
		mutex_guard grd(d->mtx);
		++d->writers;
		if (d->cap==0)
		{
			// Synchronous channel
			while (d->sync != nullptr && d->state == tchan_state::OPEN)
				d->cwr.wait(d->mtx);
			if (d->state == tchan_state::OPEN)
			{
				T local_sync(v);
				d->sync = &local_sync;
				notify_readers();
				while (d->sync != nullptr && d->state == tchan_state::OPEN)
					d->cwr.wait(d->mtx);
				d->cwr.notify_one();
			}
		}
		else
		{
			// Asynchronous channel
			while (d->q.size() == d->cap && d->state == tchan_state::OPEN)
				d->cwr.wait(d->mtx);
			if (d->state == tchan_state::OPEN)
				d->q.push(v);
			notify_readers();
		}
		--d->writers;		
	}

	template <typename T>
	void chan<T>::send(T &&v)
	{
		if (d->get_state() == tchan_state::CLOSED)
			throw exceptions::bad_closed();
		mutex_guard grd(d->mtx);
		++d->writers;
		if (d->cap == 0)
		{
			// Synchronous channel
			while (d->sync != nullptr && d->state == tchan_state::OPEN)
				d->cwr.wait(d->mtx);
			if (d->state == tchan_state::OPEN)
			{
				d->sync = &v;
				notify_readers();
				while (d->sync != nullptr && d->state == tchan_state::OPEN)
					d->cwr.wait(d->mtx);
				d->cwr.notify_one();
			}
		}
		else
		{
			// Asynchronous channel
			while (d->q.size() == d->cap && d->state == tchan_state::OPEN)
				d->cwr.wait(d->mtx);
			if (d->state == tchan_state::OPEN)
				d->q.push(std::move(v));
			notify_readers();
		}
		--d->writers;
	}

	template <typename T>
	bool chan<T>::try_send(const T& v)
	{
		if (d->get_state() == tchan_state::CLOSED)
			throw exceptions::bad_closed();
		mutex_guard grd(d->mtx);
		if (d->q.size() == d->cap)
			return false;
		d->q.push(std::move(v));
		return true;
	}

	template <typename T>
	void chan<T>::recv(T& v)
	{
		mutex_guard grd(d->mtx);
		++d->readers;
		if (d->cap==0)
		{
			// Synchronous channel
			while (d->sync == nullptr && d->state== tchan_state::OPEN)
				d->crd.wait(d->mtx);
			if (d->state == tchan_state::OPEN)
				v = std::move(*d->sync);
			d->sync = nullptr;
		}
		else
		{
			// Asynchronous channel
			while (d->q.size() == 0 && d->state == tchan_state::OPEN)
				d->crd.wait(d->mtx);
			if (d->state == tchan_state::OPEN)
				d->q.pop(v);
		}
		--d->readers;
		d->cwr.notify_one();	
	}

	template <typename T>
	bool chan<T>::try_recv(T& v)
	{
		mutex_guard grd(d->mtx);
		if (d->cap == 0)
		{
			// Synchronous channel
			if (d->sync == nullptr || d->state == tchan_state::CLOSED)
				return false;
			v = std::move(*d->sync);
			d->sync = nullptr;
		}
		else
		{
			// Asynchronous channel
			if (d->q.size() == 0 || d->state == tchan_state::CLOSED)
				return false;
			d->q.pop(v);
		}
		d->cwr.notify_one();
		return true;
	}

	template <typename T>
	void chan<T>::destroy()
	{
		--d->refs;
		if (d->refs == 0)
		{
			delete d;
		}
		d = nullptr;
	}

	template <typename T>
	void chan<T>::notify_readers()
	{
		if (d->readers > 0)
			d->crd.notify_one();
		else
		{
			if (!d->waiters.empty())
				d->waiters.front()->v();
		}
	}

	template <typename T>
	tchan_state chan<T>::get_state()
	{
		return d->state;
	}

	template <typename T>
	int chan<T>::get_buf_size()
	{
		if (d->cap>0)
			return d->q.size();
		return d->sync == nullptr ? 0 : 1;
	}

	template <typename T>
	int chan<T>::get_buf_capacity()
	{
		return d->cap;
	}

	template <typename T>
	bool chan<T>::is_same(ichannel* rhs)
	{
		return (d == rhs->get_d());
	}

	template <typename T>
	mutex *chan<T>::get_mutex()
	{
		return &d->mtx;
	}

	template <typename T>
	condvar *chan<T>::get_rcond()
	{
		return &d->crd;
	}

	template <typename T>
	void chan<T>::add_waiter(sem* cv)
	{
		d->waiters.push_back(cv);
	}

	template <typename T>
	void chan<T>::del_waiter(sem* cv)
	{
		mutex_guard grd(d->mtx);
		d->waiters.remove(cv);
	}

	template <typename T>
	void* chan<T>::get_d()
	{
		return d;
	}
}

#endif // _CHAN_H_INCLUDED_2016_05_30