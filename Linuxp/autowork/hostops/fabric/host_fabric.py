#!/usr/bin/env python

#fab -f host_fabric.py [-H 192.168.1.22,192.168.1.23,192.168.1.24] [-l list] [-- 'cmd'] go
#fab [-f host_fabric.py] [-g 192.168.1.21] [-R role] [-t local-timeout] [-T remote-timeout] [-w] [-P asyn] host_type

import time
from fabric.api import *
from fabric.colors import *
from fabric.context_managers import *
from fabric.contrib.console import confirm

env.user = 'root'
env.gateway = '192.168.1.21'
env.hosts = ['192.168.1.22', '192.168.1.23', '192.168.1.24']
env.passwords = {
    'root@192.168.1.21:22': 'SKJh935yft#',
    'root@192.168.1.22:22': 'SKJh935yft#',
    'root@192.168.1.23:22': 'KJSD9325hgs'
    'root@192.168.1.24:22': 'RTOU9325opu'
}
env.roledefs = {
    'webservers': ['192.168.1.21', '192.168.1.22'],
    'dbservers': ['192.168.1.23']
}
env.project_dev_source = '/data/dev/Lwebadmin/'
env.project_tar_source = '/data/dev/releases/'
env.project_pack_name = 'release'
env.deploy_project_root = '/data/www/Lwebadmin/'
env.deploy_release_dir = 'releases' 
env.deploy_current_dir = 'current'
env.deploy_version = time.strftime("%Y%m%d") + "v2"

@runs_once
def local_task():
    local("uname -a")

@task
def remote_task():
    dirname = prompt("please input directory name:", default="/home")
    with cd("/tmp"):
        run("ls -l " + dirname)

@task
@runs_once
def tar_task():
    with lcd("/data/logs"):
        local("tar -czf access.tar.gz access.log")

@task
def put_task():
    run("mkdir -p /data/logs")
    with cd("/data/logs"):
        with settings(warn_only=True):
            result = put("/data/logs/access.tar.gz", "/data/logs/access.tar.gz")
        if result.failed and not confirm("put file failed, Continue[Y/N]?"):
            abort("Aborting file put task!")

@task
def check_task():
    with settings(warn_only=True):
        lmd5 = local("md5sum /data/logs/access.tar.gz", capture=True).split(' ')[0]
        rmd5 = run("md5sum /data/logs/access.tar.gz").split(' ')[0]
    if lmd5 == rmd5:
        print "OK"
    else:
        print "ERROR"

@task
def run_task():
    with cd("/data/logs"):
        run("tar -zxvf access.tar.gz")
        with cd("access/"):
            run("./parse_log.sh")

@roles('webservers')
def webtask():
    print yellow("Install nginx php php-fpm...")
    with settings(warn_only=True):
        run("yum -y install nginx")
        run("yum -y install php-fpm php-mysql php-mbstring php-xml php-mcrypt php-gd")
        run("chkconfig --levels 235 php-fpm on")
        run("chkconfig --levels 235 nginx on")

@roles('dbservers')
def dbtask():
    print yellow("Install Mysql...")
    with settings(warn_only=True):
        run("yum -y install mysql mysql-server")
        run("chkconfig --levels 235 mysqld on")

@roles ('webservers', 'dbservers')
def publictask():
    print yellow("Install epel ntp...")
    with settings(warn_only=True):
        run("rpm -Uvh http://dl.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm")
        run("yum -y install ntp")

def deploy_service():
    execute(publictask)
    execute(webtask)
    execute(dbtask)

@runs_once
def input_versionid():
    return prompt("please input project rollback version ID:", default="")

@task
@runs_once
def tar_source():
    print yellow("Creating source package...")
    with lcd(env.project_dev_source):
        local("tar -czf %s.tar.gz ." % (env.project_tar_source + env.project_pack_name))
    print green("Creating source package success!")


@task
def put_package():
    print yellow("Start put package...")
    with settings(warn_only=True):
        with cd(env.deploy_project_root + env.deploy_release_dir):
            run("mkdir %s" % (env.deploy_version))
    env.deploy_full_path=env.deploy_project_root + env.deploy_release_dir + "/" + env.deploy_version

    with settings(warn_only=True):
        result = put(env.project_tar_source + env.project_pack_name +".tar.gz", env.deploy_full_path)
    if result.failed and no("put file failed, Continue[Y/N]?"):
        abort("Aborting file put task!")

    with cd(env.deploy_full_path):
        run("tar -zxvf %s.tar.gz" % (env.project_pack_name))
        run("rm -rf %s.tar.gz" % (env.project_pack_name))
    print green("Put & untar package success!")

@task
def make_symlink():
    print yellow("update current symlink")
    env.deploy_full_path = env.deploy_project_root + env.deploy_release_dir + "/" + env.deploy_version
    with settings(warn_only=True):
        run("rm -rf %s" % (env.deploy_project_root + env.deploy_current_dir))
        run("ln -s %s %s" % (env.deploy_full_path, env.deploy_project_root + env.deploy_current_dir))
    print green("make symlink success!")

@task
def rollback():
    print yellow("rollback project version")
    versionid = input_versionid()
    if versionid == '':
        abort("Project version ID error,abort!")

    env.deploy_full_path = env.deploy_project_root + env.deploy_release_dir + "/" + versionid
    run("rm -f %s" % env.deploy_project_root + env.deploy_current_dir)
    run("ln -s %s %s" % (env.deploy_full_path, env.deploy_project_root + env.deploy_current_dir)) 
    print green("rollback success!")

def deploy_app():
    tar_source()
    put_package()
    make_symlink()

@task
def go():
    local_task()
    remote_task()
    tar_task()
    put_task()
    check_task()
    run_task()
    deploy_service()
    deploy_app()
