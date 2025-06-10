#include <stdio.h>
#include "capture.h"


int main(int argc, char *argv[])
{
    
    int result = capture_frame(USB_CAMERA_PATH,OUTPUT_PICTURE);
    if (result < 0)
    {
        printf("have error!!!!!!!\n");
    }
    else
    {
        printf("success!!!!!!!\n");
    }
    
    
    return 0;
}
