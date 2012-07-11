/*
 *  ruby-sanspleur
 *
 *  Copyright 2010-2012 Fotonauts. All rights reserved.
 *
 */

#ifndef GENERIC_TICKER
#define GENERIC_TICKER

class GenericTicker {
	public:
		virtual double time_since_anchor() = 0;
		virtual long long ticks_since_anchor() = 0;
		virtual long long total_tick_count() = 0;
		
		virtual void start() = 0;
		virtual void stop() = 0;
		virtual void pause() = 0;
		virtual void resume() = 0;
		virtual void reset() = 0;
		virtual void sync_anchor() = 0;
};

#endif
