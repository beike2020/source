#!/usr/bin/env python

import re
from mrjob.job import MRJob

class MRFlowCounter(MRJob):
    def mapper(self, key, line):
        i = 0
        for flow in line.split():
            if i == 3:
                timerow = flow.split(":")
                hm = timerow[1] + ":" + timerow[2]
            if i == 9 and re.match(r"\d{1,}", flow):
                yield hm, int(flow) 
            i += 1

    def reducer(self, key, occurrences):
        yield key, sum(occurrences)


class MRConnCounter(MRJob):
    def mapper(self, key, line):
        i = 0
        for dt in line.split():
            if i == 3:
                timerow = dt.split(":")
                hm = timerow[1]+":"+timerow[2]
                yield hm, 1
            i += 1

    def reducer(self, key, occurrences):
        yield key, sum(occurrences)


class MRIPCounter(MRJob):
    IP_RE = re.compile(r"\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}")
    def mapper(self, key, line):
        for ip in IP_RE.findall(line):
            yield ip, 1

    def reducer(self, ip, occurrences):
        yield ip, sum(occurrences)


class MRURLCounter(MRJob):
    def mapper(self, key, line):
        i = 0
        for url in line.split():
            if i == 6:
                yield url, 1 
        i += 1

    def reducer(self, url, occurrences):
        yield url, sum(occurrences)


class MRStatusCounter(MRJob):
    def mapper(self, key, line):
        i = 0
        for httpcode in line.split():
            if i == 8 and re.match(r"\d{1,3}", httpcode):
                yield httpcode, 1
            i += 1

    def reducer(self, httpcode, occurrences):
        yield httpcode, sum(occurrences)

    def reducer_sorted(self, httpcode, occurrences):
        yield httpcode, sorted(occurrences)

    def steps(self):
        return [self.mr(mapper=self.mapper), self.mr(reducer=self.reducer)]


if __name__ == '__main__':
    MRFlowCounter.run()
    MRConnCounter.run()
    MRIPCounter.run()
    MRURLCounter.run()
    MRStatusCounter.run()
