// taken from https://tbz533.blogspot.com/2015/11/eta-for-c.html

#include <ctime>
#include <cmath> // floor
#include <iostream>

class EtaEstimator {
public:
    EtaEstimator(int N) : ct(0.0), etl(0.0), n(0), N(N) {
	tick = clock();
    }

    // constuction starts the clock. Pass the number of steps
    void update() {
	clock_t dt = clock() - tick;
	tick += dt;
	ct += (double(dt)/CLOCKS_PER_SEC); // prevent integer division
	// CLOCKS_PER_SEC is defined in ctime
	++n;
	etl = (ct/n) * (N-n);
    }

    void print(std::ostream & os) const {
	double etlprime = etl;
	int days = floor(etlprime / secperday);
	etlprime -= days * secperday;
	int hours = floor(etlprime / secperhour); 
	etlprime -= hours * secperhour;
	int minutes = floor(etlprime / secperminute);
	etlprime -= minutes * secperminute;
	int seconds = floor(etlprime);
	os << (days > 0 ? std::to_string(days) + " " : "")
	   << hours << ":" 
	   << (minutes < 10 ? "0" : "") << minutes << ":" 
	   << (seconds < 10 ? "0" : "") << seconds;
    }

private:
    double ct, etl; // cumulative time, estimated time left
    int n, N; // steps taken, total amount of steps
    clock_t tick; // time after update ((c) matlab)
    // statics...
    static const int secperday = 86400;
    static const int secperhour = 3600;
    static const int secperminute = 60;
};

std::ostream & operator<<(std::ostream & os,
        const EtaEstimator & eta) {
    eta.print(os);
    return os;
}
