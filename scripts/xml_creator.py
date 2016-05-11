import os,sys,subprocess,shutil,errno



tasks = []
tasks.append(dict(task="GLManagerTask",          lib="GraphixTask",        path=""))
tasks.append(dict(task="GLObjApp",               lib="GraphixPeer",        path=""))
tasks.append(dict(task="UDPReceiverTask",        lib="UDPReceiver",        path=""))
tasks.append(dict(task="UDPCollisionDataTask",   lib="UDPServer",          path=""))
tasks.append(dict(task="UDPHMDDataTask",         lib="UDPServer",          path=""))
tasks.append(dict(task="FlannIndexTask",         lib="IndexBuilder",       path=""))
tasks.append(dict(task="CollisionDetectionTask", lib="CollisionDetection", path=""))
tasks.append(dict(task="FrameHandlerTask",       lib="FrameHandler",       path=""))

tasks.append(dict(task="LeapControllerTask",     lib="LeapController",     path=""))
tasks.append(dict(task="ReceiverTask",           lib="ReceiverTask",       path=""))
tasks.append(dict(task="ServerTask",             lib="ServerTask",         path=""))


def parseTask(task):
    if task["path"] == "" :
        path = lib_path
    else:
        path = task["path"]
    command = "%s %s %s %s" % (app, task["task"], task["lib"], path)
    subprocess.call(command, shell=True)



if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description='Dependency Maker')
    parser.add_argument('--path', help="Specify Library Path")
    parser.add_argument('--app',  help="Specify executable")
    parser.add_argument('--task', help="Specify for which task to print xml")
    args = parser.parse_args()
    lib_path = args.path
    app = args.app
    if app == None :
        app = "../build/bin/cocoARapp"
    task = args.task
    if task != None:
        for t in tasks:
            if t["task"] == task :
                tt = t 
        parseTask(tt)
    else :
        for task in tasks :
            parseTask(task)

    #libraryPath = args.lib

