/*************************************************************************
	> File Name: main.cpp
	> Author: 
	> Mail: 
	> Created Time: 2016年02月17日 星期三 22时01分17秒
 ************************************************************************/

#include <iostream>
#include "CThreadManage.h"
#include "CRealJob.h"

void doTask(void* jobData)
{
    std::cout << "Hello world" << std::endl;
    boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
}

int main()
{
    CThreadManagePtr manage(new CThreadManage);
    manage->initThreadNum(10);

    for (int i = 0; i < 13; ++i)
    {
        CRealJobPtr job(new CRealJob);
        job->setJob(boost::bind(doTask, _1));
        manage->run(job, NULL);
    }

    boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));

    std::cout << "#################################" << std::endl;
    for (int i = 0; i < 17; ++i)
    {
        CRealJobPtr job(new CRealJob);
        manage->run(job, NULL);
    }

    boost::this_thread::sleep_for(boost::chrono::milliseconds(10000));

    for (int i = 0; i < 3; ++i)
    {
        CRealJobPtr job(new CRealJob);
        manage->run(job, NULL);
    }

    boost::this_thread::sleep_for(boost::chrono::milliseconds(10000));

    for (int i = 0; i < 25; ++i)
    {
        CRealJobPtr job(new CRealJob);
        manage->run(job, NULL);
    }
#if 1
    boost::this_thread::sleep_for(boost::chrono::milliseconds(10000));

    for (int i = 0; i < 35; ++i)
    {
        CRealJobPtr job(new CRealJob);
        manage->run(job, NULL);
    }
#endif
    boost::this_thread::sleep_for(boost::chrono::milliseconds(10000));

    return 0;
}
