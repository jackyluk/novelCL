/*!****************************************************************************
 * @file ComputeUnit.cpp ComputeUnit implementation
 * @author Jacky H T Luk 2013
 *****************************************************************************/

#include "ComputeUnit.hpp"
#include "debug.h"

/*!****************************************************************************
 * @brief Compute Unit constructor
 * @param parent Reference to an IScheduler
 * @param designation ComputeUnit Unique ID
 * @param dataPtr Pointer to data memory.
 *****************************************************************************/
ComputeUnit::ComputeUnit(IScheduler *parent, int designation, 
                         char *dataPtr){
    this->data = dataPtr;
    this->parent = parent;
    this->thread = 0;
    this->designation = designation;
    this->dlHandle = NULL;
}

/*!****************************************************************************
 * @brief Set kernel to be executed
 * @param lib_name Kernel file name
 *****************************************************************************/
void ComputeUnit::set_kernel(char *lib_name){
    char* error;
    
    if(this->dlHandle){
      dlclose(this->dlHandle);
      this->dlHandle = NULL;
    }
    /* link with kernel compiled as shared library */
    //fprintf(stderr, "Opening %s\n", lib_name);
    this->dlHandle = dlopen(lib_name, RTLD_NOW);
    if (!this->dlHandle) {
        DEBUG("%s\n", dlerror());
        exit(EXIT_FAILURE);
    }
    this->pfnKernelWrapper = (void (*)(int, int, int, void *))dlsym(this->dlHandle, "kernel_wrapper");
    dlerror(); /* clear previous error */
    if ((error = dlerror()) != NULL)  {
        DEBUG("%s\n", dlerror());
        exit(EXIT_FAILURE);
    }
}

/*!****************************************************************************
 * @brief Start kernel execution
 * @param z 3rd dimension of execution
 * @param y 2nd dimension of execution
 * @param x 1st dimension of execution
 *****************************************************************************/
void ComputeUnit::run_kernel(int z, int y, int x){
    char* error;
    this->globalX = x;
    this->globalY = y;
    this->globalZ = z;

    //Start the thread
    DEBUG("Calling Designation %d, Instance %d:%d:%d\n", this->designation, z, y, x);
    if(this->thread){
        pthread_join(this->thread, NULL);
        this->thread = 0;
    }
    pthread_create(&(this->thread), NULL, cu_thread_start, this);
}

/*!****************************************************************************
 * @brief Start of thread function, required for pthread
 * @param arg Pointer to an instance of ComputeUnit
 *****************************************************************************/
void *ComputeUnit::cu_thread_start(void *arg){
  return ((ComputeUnit *)arg)->cu_thread();
}

/*!****************************************************************************
 * @brief CU Thread
 *****************************************************************************/
void* ComputeUnit::cu_thread(){
  this->pfnKernelWrapper(this->globalX, this->globalY, this->globalZ, this->data);
  dlclose(this->dlHandle);
  this->dlHandle = NULL;
  DEBUG("%d %d:%d:%d EXD\n", this->designation, this->globalZ, this->globalY, this->globalX);
  this->parent->CUDone(this);
}

/*!****************************************************************************
 * @brief Compute Unit Join
 *****************************************************************************/
void ComputeUnit::join(){
    if(this->thread){
        pthread_join(this->thread, NULL);
        this->thread = 0;
    }
}

/*!****************************************************************************
 * @brief Compute Unit Destructor
 *****************************************************************************/
ComputeUnit::~ComputeUnit(){
  if(this->dlHandle){
    dlclose(this->dlHandle);
    this->dlHandle = NULL;
  }
}
