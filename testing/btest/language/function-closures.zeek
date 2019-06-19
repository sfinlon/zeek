function make_count_upper (start : count) : function(step : count) : count
	{
	return function(step : count) : count
		{ return (start += step); };
        }

event zeek_init()
	{
	# basic
	local one = make_count_upper(1);
	print "expect: 3";
	print one(2);

	# multiple instances
	local two = make_count_upper(one(1));
	print "expect: 5";
	print two(1);
	print "expect: 5";
	print one(1);

	# deep copies
	local c = copy(one);
	print "expect: T";
	print c(1) == one(1);
	print "expect: T";
	print c(1) == two(2);
	
	# a little more complicated ...
	local cat_dog = 100;
	local add_n_and_m = function(n: count) : function(m : count) : function(o : count) : count
		{
		return function(m : count) : function(o : count) : count
			{ return function(o : count) : count
				{ return  n + m + o + cat_dog; }; };
		};

	local add_m = add_n_and_m(2);
	local adder = add_m(2);
	
	print "expect: 6";	
	print adder(2);

	# deep copies
	local ac = copy(adder);
	print ac(2);
	}

