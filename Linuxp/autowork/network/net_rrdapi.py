# -*- coding: utf-8 -*-
#!/usr/bin/python
import time
import psutil
import rrdtool

def rrd_create():
    cur_time=str(int(time.time()))
    rrd=rrdtool.create('/root/net_flowes.rrd','--step','300','--start',cur_time,
        'DS:eth0_in:COUNTER:600:0:U',
        'DS:eth0_out:COUNTER:600:0:U',
        'RRA:AVERAGE:0.5:1:600',
        'RRA:AVERAGE:0.5:6:700',
        'RRA:AVERAGE:0.5:24:775',
        'RRA:AVERAGE:0.5:288:797',
        'RRA:MAX:0.5:1:600',
        'RRA:MAX:0.5:6:700',
        'RRA:MAX:0.5:24:775',
        'RRA:MAX:0.5:444:797',
        'RRA:MIN:0.5:1:600',
        'RRA:MIN:0.5:6:700',
        'RRA:MIN:0.5:24:775',
        'RRA:MIN:0.5:444:797')
    if rrd:
        print rrdtool.error()


def rrd_update():
    total_input_traffic = psutil.net_io_counters()[1]
    total_output_traffic = psutil.net_io_counters()[0]
    starttime=int(time.time())
    update=rrdtool.updatev('/root/net_flowes.rrd','%s:%s:%s' % (str(starttime),str(total_input_traffic),str(total_output_traffic)))
    print update 


def rrd_graph():
    title="Server network  traffic flow ("+time.strftime('%Y-%m-%d',time.localtime(time.time()))+")"
    rrdtool.graph( "/root/net_flowes.png", "--start", "-1d","--vertical-label=Bytes/s","--x-grid","MINUTE:12:HOUR:1:HOUR:1:0:%H",\
        "--width","650","--height","230","--title",title,
        "DEF:inoctets=net_flowes.rrd:eth0_in:AVERAGE",
        "DEF:outoctets=net_flowes.rrd:eth0_out:AVERAGE",
        "CDEF:total=inoctets,outoctets,+",
        "LINE1:total#FF8833:Total traffic",
        "AREA:inoctets#00FF00:In traffic",
        "LINE1:outoctets#0000FF:Out traffic",
        "HRULE:6144#FF0000:Alarm value\\r",
        "CDEF:inbits=inoctets,8,*",
        "CDEF:outbits=outoctets,8,*",
        "COMMENT:\\r",
        "COMMENT:\\r",
        "GPRINT:inbits:AVERAGE:Avg In traffic\: %6.2lf %Sbps",
        "COMMENT:   ",
        "GPRINT:inbits:MAX:Max In traffic\: %6.2lf %Sbps",
        "COMMENT:  ",
        "GPRINT:inbits:MIN:MIN In traffic\: %6.2lf %Sbps\\r",
        "COMMENT: ",
        "GPRINT:outbits:AVERAGE:Avg Out traffic\: %6.2lf %Sbps",
        "COMMENT: ",
        "GPRINT:outbits:MAX:Max Out traffic\: %6.2lf %Sbps",
        "COMMENT: ",
        "GPRINT:outbits:MIN:MIN Out traffic\: %6.2lf %Sbps\\r")

