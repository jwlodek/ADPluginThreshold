/**
 * A basic template for implementing areaDetector plugins.
 * 
 * Author: Jakub Wlodek
 * Created on: 12/31/2024
 * 
 */

// Standard library headers
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>


// EPICS/AreaDetector headers
#include <epicsMutex.h>
#include <epicsString.h>
#include <iocsh.h>
#include "NDArray.h"
#include <epicsExport.h>

// Plugin header
#include "NDPluginThreshold.hpp"


// Error message formatters
#define ERR(msg)                                                                                 \
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "ERROR | %s::%s: %s\n", pluginName, functionName, \
              msg)

#define ERR_ARGS(fmt, ...)                                                              \
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "ERROR | %s::%s: " fmt "\n", pluginName, \
              functionName, __VA_ARGS__);

// Warning message formatters
#define WARN(msg) \
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "WARN | %s::%s: %s\n", pluginName, functionName, msg)

#define WARN_ARGS(fmt, ...)                                                            \
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "WARN | %s::%s: " fmt "\n", pluginName, \
              functionName, __VA_ARGS__);

// Log message formatters (set to use ERROR level by default they are printed w/ default trace settings.)
#define LOG(msg) \
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: %s\n", pluginName, functionName, msg)

#define LOG_ARGS(fmt, ...)                                                                       \
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s::%s: " fmt "\n", pluginName, functionName, \
              __VA_ARGS__);


// Include your external dependency library headers


// Namespaces
using namespace std;


// Name of the plugin
static const char *pluginName="NDPluginThreshold";


/**
 * Override of NDPluginDriver function. Must be implemented by your plugin
 *
 * Performs callback when write operation is performed on an asynInt32 record
 * 
 * @params[in]: pasynUser   -> pointer to asyn User that initiated the transaction
 * @params[in]: value       -> value PV was set to
 * @return: success if PV was updated correctly, otherwise error
 */
asynStatus NDPluginThreshold::writeInt32(asynUser* pasynUser, epicsInt32 value){
    const char* functionName = "writeInt32";
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    // TODO: Handle callbacks for any integer PV writes as needed here

    // Check to see if threshold value requested is reasonable
    if(function == NDPluginThreshold_Value && (value < 0 || value > 255)) {
        ERR("Threshold value must be between 0 and 255!");
        status = asynError;
    }

    if (function >= ND_THRESHOLD_FIRST_PARAM && status != asynError){
        status = setIntegerParam(function, value);
    } else if(function < ND_THRESHOLD_FIRST_PARAM){
        status = NDPluginDriver::writeInt32(pasynUser, value);
    }

    callParamCallbacks();

    if(status){
        ERR_ARGS("Failed to write int32 val to parameter: function = %d value=%d", function, value);
    }

    return status;
}


/**
 * Override of NDPluginDriver function. Must be implemented by your plugin
 *
 * Performs callback when write operation is performed on an asynFloat64 record
 * 
 * @params[in]: pasynUser   -> pointer to asyn User that initiated the transaction
 * @params[in]: value       -> value PV was set to
 * @return: success if PV was updated correctly, otherwise error
 */
asynStatus NDPluginThreshold::writeFloat64(asynUser* pasynUser, epicsFloat64 value){
    const char* functionName = "writeFloat64";
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    // TODO: Handle callbacks for any integer PV writes as needed here

    status = setDoubleParam(function, value);

    if(function < ND_THRESHOLD_FIRST_PARAM){
        status = NDPluginDriver::writeFloat64(pasynUser, value);
    }

    callParamCallbacks();

    if(status){
        ERR_ARGS("Failed to write float64 val to parameter: function = %d value=%f", function, value);
    }

    return status;
}



/* Process callbacks function inherited from NDPluginDriver.
 * You must implement this function for your plugin to accept NDArrays
 *
 * @params[in]: pArray -> NDArray recieved by the plugin from the camera
 * @return: void
*/
void NDPluginThreshold::processCallbacks(NDArray *pArray){
    static const char* functionName = "processCallbacks";
    NDArray *pScratch;
    asynStatus status = asynSuccess;
    NDArrayInfo arrayInfo;

    // If set to true, downstream plugins will perform callbacks on output pScratch
    // If false, no downstream callbacks will be performed
    bool performCallbacks = true;

    //call base class and get information about frame
    NDPluginDriver::beginProcessCallbacks(pArray);

    pArray->getInfo(&arrayInfo);

    LOG_ARGS("Processing frame w/ ID %d", pArray->uniqueId);

    //unlock the mutex for the processing portion
    this->unlock();

    // This sets the output of the plugin to the input array
    pScratch = this->pNDArrayPool->copy(pArray, NULL, 1);

    // If we are manipulating the image/output, we allocate a new scratch frame instead of copying.
    // You will need to specify dimensions, and data type.
    //pScratch = pNDArrayPool->alloc(ndims, dims, dataType, 0, NULL);

    if(pScratch == NULL){
        ERR("Unable to allocate frame.");
        return;
    }

    // Process the image here.
    // 
    // Note that this expects any external libraries to be thread safe. If they aren't, move
    // the processing to after this->lock();
    //
    // Access data with pArray->pData.
    // DO NOT CALL pArray.release()

    NDColorMode_t colorMode;
    NDDataType_t dataType;
    getIntegerParam(NDColorMode, (int*) &colorMode);
    getIntegerParam(NDDataType, (int*) &dataType);

    int thresholdValue;
    getIntegerParam(NDPluginThreshold_Value, &thresholdValue);

    if(colorMode != NDColorModeMono || dataType != NDUInt8) {
        ERR("Only Monochrome 8 bit images supported!");
    } else {
        int numPixelsUnderThresh = 0;
        for(size_t i = 0; i< arrayInfo.totalBytes; i++){
            uint8_t pixelValue = *((uint8_t*) pArray->pData + i);
            if (pixelValue < thresholdValue) {
                numPixelsUnderThresh++;
                *((uint8_t*) pScratch->pData + i) = 0;
            }
            else{
                *((uint8_t*) pScratch->pData + i) = 255;
            }
        }
        setIntegerParam(NDPluginThreshold_NumPixThresh, numPixelsUnderThresh);
    }

    this->lock();

    // If pScratch was allocated, set the color mode, unique ID, and timestamp here.

    //pScratch->pAttributeList->add("ColorMode", "Color Mode", NDAttrInt32, &colorMode);
    //pScratch->uniqueId = pArray->uniqueId;
    //pScratch->epicsTS = pArray->epicsTS;

    if(status == asynError){
        ERR("Image not processed correctly!");
        return;
    }

    if(pScratch != NULL)
        NDPluginDriver::endProcessCallbacks(pScratch, false, performCallbacks);

    callParamCallbacks();
}



//constructror from base class, replace with your plugin name
NDPluginThreshold::NDPluginThreshold(
        const char *portName, int queueSize, int blockingCallbacks,
        const char *NDArrayPort, int NDArrayAddr,
        int maxBuffers, size_t maxMemory,
        int priority, int stackSize, int maxThreads)
        /* Invoke the base class constructor */
        : NDPluginDriver(portName, queueSize, blockingCallbacks,
        NDArrayPort, NDArrayAddr, 1, maxBuffers, maxMemory,
        asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
        asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
        ASYN_MULTIDEVICE, 1, priority, stackSize, maxThreads)
{
    const char* functionName = "NDPluginThreshold";

    char versionString[25];

    // Initialize Parameters here, using the string vals and indexes from the header. Ex:
    createParam(NDPluginThresholdStatusString, asynParamInt32,   &NDPluginThreshold_Status);
    createParam(NDPluginThresholdNumPixThreshString, asynParamInt32,   &NDPluginThreshold_NumPixThresh);
    createParam(NDPluginThresholdValueString, asynParamInt32,   &NDPluginThreshold_Value);

    // Update plugin version number parameter
    setStringParam(NDPluginDriverPluginType, "NDPluginThreshold");
    epicsSnprintf(versionString, sizeof(versionString), "%d.%d.%d", 
                  THRESHOLD_VERSION, 
                  THRESHOLD_REVISION, 
                  THRESHOLD_MODIFICATION);
    setStringParam(NDDriverVersion, versionString);

    connectToArrayPort();

    LOG_ARGS("Initialization complete. Version: %s", versionString);
}



/**
 * External configure function. This will be called from the IOC shell of the
 * detector the plugin is attached to, and will create an instance of the plugin and start it
 * 
 * @params[in]	-> all passed to constructor
 */
extern "C" int NDThresholdConfigure(
        const char *portName, int queueSize, int blockingCallbacks,
        const char *NDArrayPort, int NDArrayAddr,
        int maxBuffers, size_t maxMemory,
        int priority, int stackSize, int maxThreads){

    // Initialize instance of our plugin and start it.
    NDPluginThreshold *pPlugin = new NDPluginThreshold(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, maxBuffers, maxMemory, priority, stackSize, maxThreads);
    return pPlugin->start();
}


/* IOC shell arguments passed to the plugin configure function */
static const iocshArg initArg0 = { "portName", iocshArgString };
static const iocshArg initArg1 = { "frame queue size", iocshArgInt };
static const iocshArg initArg2 = { "blocking callbacks", iocshArgInt };
static const iocshArg initArg3 = { "NDArrayPort", iocshArgString };
static const iocshArg initArg4 = { "NDArrayAddr", iocshArgInt };
static const iocshArg initArg5 = { "maxBuffers", iocshArgInt };
static const iocshArg initArg6 = { "maxMemory", iocshArgInt };
static const iocshArg initArg7 = { "priority", iocshArgInt };
static const iocshArg initArg8 = { "stackSize", iocshArgInt };
static const iocshArg initArg9 = { "maxThreads", iocshArgInt };
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8,
                                            &initArg9};


// Define the path to your plugin's extern configure function above
static const iocshFuncDef initFuncDef = { "NDThresholdConfigure", 10, initArgs };


/* link the configure function with the passed args, and call it from the IOC shell */
static void initCallFunc(const iocshArgBuf *args){
    NDThresholdConfigure(
            args[0].sval, args[1].ival, args[2].ival,
            args[3].sval, args[4].ival, args[5].ival,
            args[6].ival, args[7].ival, args[8].ival, args[9].ival);
}


/* function to register the configure function in the IOC shell */
extern "C" void NDThresholdRegister(void){
    iocshRegister(&initFuncDef,initCallFunc);
}


/* Exports plugin registration */
extern "C" {
    epicsExportRegistrar(NDThresholdRegister);
}
