#include <ctime>
#include <cstdio>
#include <set>
#include "hash.h"
#include "sketch.h"
#include "parameters.h"
#include "quintet.h"

Sketch sketch;

namespace DataSyntax {

std::set <int> protocols;
std::FILE *file;
int count;

int init (const char *name) {
	count = 0;
	file = fopen (name, "rb");
	return file != NULL;
}

/**
 * Return the quintet and the delay of a package.
*/
Quintet getQuin () {
	Quintet quin;
	if (fread (&quin.SrcIP, 4, 1, file) < 1) {
		quin.delay = -1;
		fclose (file);
		file = NULL;
	}
	else {
		++count;
		assert (fread (&quin.DstIP, 4, 1, file));
		assert (fread (&quin.SrcPort, 2, 1, file)); 
		assert (fread (&quin.DstPort, 2, 1, file)); 
		assert (fread (&quin.Protocol, 1, 1, file)); 
		assert (fread (&quin.delay, 4, 1, file)); 
		protocols.insert (quin.Protocol).second;
	}
	return quin;
}

void reset () {
	if (file != NULL)
		fclose (file);
	file = NULL;
	return;
}

}

std::vector <Quintet> quinvec;

int main (int argc, char **argv) {
	assert (PERCENTAGE >= 0.5);
	assert (PERCENTAGE < 1 - 1e-3);
	const char input_name [] = "../130000_delay.dat";
	char *output_name = NULL;
	DataSyntax::init (input_name);
	for (int i = 0; ; ++i) {
		Quintet quin (DataSyntax::getQuin ());
		/** All quintets have been read and processed. */
		if (quin.delay < 0) {
			break;
		}
		quinvec.push_back (quin);
	}
	int test_cycles = 10;
	timespec time1, time2;
	double mops = 0;
	for (int i = 0; i < test_cycles; ++i) {
		long long resns = 0;
		sketch.init (argc, argv, output_name, false);
		int time = 0, sz = (int) quinvec.size ();
		clock_gettime (CLOCK_MONOTONIC, &time1); 
		for (time = 0; time < sz; ++time) {
			sketch.insert (quinvec [time], time);
		}
		clock_gettime (CLOCK_MONOTONIC, &time2);
		resns += (long long) (time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec); 
		double test_mops = (double) 1000.0 * time / (double) resns;
        mops += (test_mops - mops) / (i + 1);
		sketch.finish (mops);
	}
	sketch.init (argc, argv, output_name, true);
	int time = 0, sz = (int) quinvec.size ();
	for (time = 0; time < sz; ++time) {
		sketch.insert (quinvec [time], time);
	}
	printf ("Finished! Time = %d, Mops = %.12lf\n", time, mops);
	sketch.finish (mops);
	return 0;
}