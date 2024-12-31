/**
 * A basic template for implementing areaDetector plugins.
 * 
 * Author:Jakub Wlodek 
 * Created on: 12/31/2024
 * 
 */

#ifndef NDPluginThreshold_H
#define NDPluginThreshold_H

//Define necessary includes here

using namespace std;

//include base plugin driver
#include "NDPluginDriver.h"

//version numbers
#define THRESHOLD_VERSION          0
#define THRESHOLD_REVISION         0
#define THRESHOLD_MODIFICATION     1



// Define the strings that map your records to params here
#define NDPluginThresholdStatusString    "THR_STATUS"  //asynInt32
#define NDPluginThresholdNumPixThreshString    "THR_NUM_PIX_THRESH"  //asynInt32
#define NDPluginThresholdValueString    "THR_VALUE"  //asynInt32


/* Plugin class, extends NDPluginDriver base class */
class NDPluginThreshold : public NDPluginDriver {
    public:
        NDPluginThreshold(const char *portName, int queueSize, int blockingCallbacks,
            const char* NDArrayPort, int NDArrayAddr, int maxBuffers,
            size_t maxMemory, int priority, int stackSize, int maxThreads);


        void processCallbacks(NDArray *pArray);

        virtual asynStatus writeInt32(asynUser* pasynUser, epicsInt32 value);
        virtual asynStatus writeFloat64(asynUser* pasynUser, epicsFloat64 value);

    protected:

        // Define the Param index variables here. Ex:
        int NDPluginThreshold_Status;
        int NDPluginThreshold_NumPixThresh;
        int NDPluginThreshold_Value;

        // Define these two variables as the first and last param indexes.
        #define ND_THRESHOLD_FIRST_PARAM NDPluginThreshold_Status
        #define ND_THRESHOLD_LAST_PARAM NDPluginThreshold_Value

    private:

        // init all global variables here

        // init all plugin additional functions here

};

// Def that computes the number of params specific to the plugin
#define NUM_THRESHOLD_PARAMS ((int)(&ND_THRESHOLD_LAST_PARAM - &ND_THRESHOLD_FIRST_PARAM + 1))

#endif
