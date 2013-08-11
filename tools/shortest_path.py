import os;
import ah_config;

pParameter = ah_config.pParameter
executables = ah_config.executables 


def runShortestPath(queues, threads, maxThreads, directory=''):

  for queue in queues:

    for thread in threads:

        exe = queue
        if queue in executables:
          exe = executables[queue]

        template = "../ah-queues/scal/shortest-path-{exe} -threads {thread} -height 100 -width 10000 {partials_param} {partials} -noset_rt_priority > {directory}{queue}-t{thread}{partials_param}{partials}.txt"

        commands = []
        if queue in pParameter:
            commands = []

            commands.extend([template.format(queue=queue,
                                             exe=exe,
                                             thread=thread, 
                                             partials_param='-' + pParameter[queue], 
                                             partials=maxThreads,
                                             directory=directory,
                                             )])
        else:

            commands = [template.format(queue=queue,
                                        exe = exe,
                                        thread=thread, 
                                        partials_param='', 
                                        partials='',
                                        directory=directory,
                                        )]

        for command in commands:
            print command
            os.system(command)

#runSeqalt(queues = ah_config.queues, works = ah_config.works, threads = ah_config.threads, maxThreads = ah_config.maxThreads)
