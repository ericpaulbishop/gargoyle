#!/usr/bin/awk -f

# There are more comments below the following BEGIN block.

BEGIN {
	# This value is in seconds.
	# It must match the -i interval used with ntpclient if not
	# the default of 10 minutes.
	td = 10 * 60;

	# How many milliseconds may a sample be off by (system time) and
	# still be counted?
	# This check (with the above td) is here to tell you if you have
	# several ntpclient processes logging to the same file.
	tderr = 100;

	# How many ppm may one sample's computed frequency differ from the
	# previous one and still be counted?
	# This check filters out records resulting from an NTP server glitch.
	freq_diff = 5;

	# How many milliseconds may it take to get an answer from the
	# ntp server?
	# Long replies often produce glitchy results.
	ntp_delay = 100;
}

# This script is to determine what your /etc/ntp.adj should be
# It is invoked thusly:
# 	awk -f rate2.awk < /var/lib/ntp.log
# Unlike the original rate.awk script supplied with ntpclient,
# this one can be run against the log of a running ntpclient which
# is adjusting the adjtimex frequency.  The alternative is a drag
# because it means you cannot be keeping time while you are determining
# a suitable adjtimex frequency.  You, of course, can do that with
# this program too; you just don't have to.  Given a file that works
# with the original rate.awk, this script should produce similar
# results.  You may have to increase the above thresholds to keep
# records from being ignored so you get the same answers.

# It is assumed that nothing besides ntpclient is adjusting that
# value for the duration of the creation of the log file.

# It is recommended that you collect lots of log entries.
# It is further recommended that you collect logs for roughly an
# integer number of days to even out daily cycles (temperature, etc.)


# No tweaking necessary below here, I hope.

function pretty_time(ss,	dd, hh, mm, ret, f) {
	ss += 0;
	if(ss < 0) {
		ret = "-";
		ss = -ss;
	} else {
		ret = "";
	}
	mm = int(ss / 60); ss -= (mm * 60);
	hh = int(mm / 60); mm -= (hh * 60);
	dd = int(hh / 24); hh -= (dd * 24);

	if(dd != 0) {
		ret = sprintf("%s%d+", ret, dd);
	}
	if(ret != "" || hh != 0) {
		ret = sprintf("%s%02d:", ret, hh);
	}
	if(ret != "" || mm != 0) {
		ret = sprintf("%s%02d:", ret, mm);
	}
	ss = sprintf("%06.3f", ss); sub(/0+$/, "", ss); sub(/\.$/, "", ss);
	if(ret != "" || (ss + 0) != 0) {
		ret = sprintf("%s%s", ret, ss);
	} else {
		ret = "0";
	}
	return ret;
}

function abs(a) {
	a += 0;
	if(a < 0) {
		return -a;
	}
	else {
		return a;
	}
}

function round(a, p) {
	p += 0;
	if(p == 0) {
		return int(a + 0.5);
	} else {
		return int((a / p) + 0.5) * p;
	}
}

BEGIN {
	f1 = "";
	tderr = tderr / 1000;
	tdmin = td - tderr;
	tdmax = td + tderr;
	freq_diff = freq_diff * 65536;
	ntp_delay = ntp_delay * 1000;
	c = 0;
	ignored = 0;
	delta_o = 0;
	delta_t = 0;
	sum_f2 = 0;
	min_f2 = "init";
	max_f2 = "init";
	prev_f2 = "init";
}

/^[0-9]/ {
	if(f1 == "") {
		t1 = $1 * 86400 + $2;
		o1 = $5;
		f1 = $7;

		t2 = ts = t1;
		o2 = o1;
		fs = f1;

		c++;
	} else {
		t2 = $1 * 86400 + $2;
		o2 = $5;

		td = t2 - t1;
		od = o2 - o1;
		if($3 > ntp_delay) {
			print "Ignoring: ntp delay (" $3 / 1000 " ms) out of spec in line " FNR ".";
			ignored++;
		} else {
			if(td < tdmin || td > tdmax) {
				print "Ignoring: time difference (" pretty_time(td) ") out of spec in line " FNR ".";
				ignored++;
			} else {
				f2 = f1 + (65536 * od / td);
				if(prev_f2 == "init") {prev_f2 = f2;}
				if(abs(f2 - prev_f2) > freq_diff) {
					print "Ignoring: target frequency adjustment (" (f2 - prev_f2) / 65536 " ppm) out of spec in line " FNR ".";
					ignored++;
				} else {
					# weigted average
					sum_f2 += f2 * td;
					delta_o += od;
					delta_t += td;

					# minimum / maximum
					if(min_f2 == "init") { min_f2 = f2; }
					else { if(f2 < min_f2) { min_f2 = f2; }}
					if(max_f2 == "init") { max_f2 = f2; }
					else { if(f2 > max_f2) { max_f2 = f2; }}

					c++;
				}
				prev_f2 = f2;
			}
		}

		t1 = t2;
		o1 = o2;
		f1 = $7;
	}
}

END {
	if(ignored > 0) {
		print "";
	}
	print "total time:",pretty_time(round(delta_t));
	print "number of samples:",c;
	if(ignored > 0) {
		print "ignored samples:", ignored, "(" round(100 * ignored / (ignored + c)) "%)";
	}
	if(delta_t >= tdmin) {
		print "";
		slope = delta_o / delta_t;
		f2 = sum_f2 / delta_t;
		print "delta-t", delta_t, "seconds"
		print "delta-o", delta_o, "useconds"
		print "slope:", slope, "ppm";
		print "old frequency:", fs, "(" fs / 65536,"ppm)";
#		print "min frequency:", int(min_f2 + .5), "(" min_f2 / 65536,"ppm)";
#		print "max frequency:", int(max_f2 + .5), "(" max_f2 / 65536,"ppm)";
		print "new frequency:", int(f2 + .5), "(" f2 / 65536,"ppm)";
	}
}

