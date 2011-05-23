#include <iostream>
#include <stdexcept>

#include <XnCppWrapper.h>

int main (int argc, char * const argv[]) {
    try {
        xn::Context context;
        XnStatus rc = context.Init();
        if ( rc != XN_STATUS_OK ) {
            throw std::runtime_error( ::xnGetStatusString( rc ) );
        }
        std::cout << "Success" << std::endl;
    }
    catch (std::exception& ex) {
        std::cout << ex.what() << std::endl;
    }
    
    return 0;
}
