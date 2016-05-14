#!/usr/bin/env python

import sys
import datetime
import subprocess
from mrjob.job import MRJob

def hdfsput():
    webid="web1"
    currdate=datetime.datetime.now().strftime('%Y%m%d')
    logspath="/data/logs/"+currdate+"/access.log"
    logname="access.log."+webid

    try:
        subprocess.Popen(["/usr/local/hadoop-1.2.1/bin/hadoop", "dfs", "-mkdir", "hdfs://192.168.1.20:9000/user/root/website.com/"+currdate], stdout=subprocess.PIPE)
    except Exception,e:
        pass
    putinfo=subprocess.Popen(["/usr/local/hadoop-1.2.1/bin/hadoop", "dfs", "-put", logspath, "hdfs://192.168.1.20:9000/user/root/website.com/"+currdate+"/"+logname], stdout=subprocess.PIPE)

    for line in putinfo.stdout:
        print line


def mapfunc():
    for line in sys.stdin:
        line = line.strip()
        words = line.split()
        for word in words:
            print '%s\t%s' % (word, 1)

def reducefunc():
    current_word = None
    current_count = 0
    word = None

    for line in sys.stdin:
        line = line.strip()
        word, count = line.split('\t', 1)
        try:
            count = int(count)
        except ValueError:
            continue

        if current_word == word:
            current_count += count
        else:
            if current_word:
                print '%s\t%s' % (current_word, current_count)
            current_count = count
            current_word = word

    if current_word == word:
        print '%s\t%s' % (current_word, current_count)


class MRMaxWordCounter(MRJob):
    WORD_RE = re.compile(r"\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}")
    def mapper(self, key, line):
        user_id   = line.split()[0]
        timestamp = line.split()[1].split(',')
        yield user_id, timestamp

    def reducer(self, uid, timestamps):
        for b in timestamps:
                yield uid, max(b)


class MRSumWordCounter(MRJob):
    def mapper(self, key, line):
        for word in line.split():
            yield word, 1

    def reducer(self, word, occurrences):
        yield word, sum(occurrences)


class MRSortWordCounter(MRJob):
    def mapper(self, key, line):
        timestamp, user_id = line.split()
        yield user_id, timestamp

    def reducer(self, uid, timestamps):
        print sorted(timestamps)
        yield uid, sorted(timestamps)


if __name__ == '__main__':
    hdfsput()
    mapfunc()
    reducefunc()
    MRMaxWordCounter.run()
    MRSumWordCounter.run()
    MRSortWordCounter.run()
