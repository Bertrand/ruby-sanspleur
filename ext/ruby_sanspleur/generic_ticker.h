/*
 *  generic_ticker.h
 *  ruby-sanspleur
 *
 *  Created by Jérôme Lebel on 30/11/10.
 *  Copyright 2010 Fotonauts. All rights reserved.
 *
 */


class GenericTicker {
	protected:
		virtual ~GenericTick() = 0;
		
	public:
		virtual double anchor_difference() = 0;
		virtual void update_anchor() = 0;
		virtual double anchor_value() = 0;
		virtual int anchor_tick_value() = 0;
		
		virtual void start() = 0;
		virtual void stop() = 0;
};