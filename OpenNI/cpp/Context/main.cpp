#include <iostream>
#include <stdexcept>

#include <XnCppWrapper.h>

// 設定ファイルのパス(環境に合わせて変更してください)
#if (XN_PLATFORM == XN_PLATFORM_WIN32)
const char* CONFIG_XML_PATH = "../../../Data/SamplesConfig.xml";
#elif (XN_PLATFORM == XN_PLATFORM_MACOSX)
const char* CONFIG_XML_PATH = "../../../../../Data/SamplesConfig.xml";
#elif (XN_PLATFORM == XN_PLATFORM_LINUX_X86)
const char* CONFIG_XML_PATH = "../../../Data/SamplesConfig.xml";
#else
const char* CONFIG_XML_PATH = "Data/SamplesConfig.xml";
#endif

int main( char* argv[], int argc )
{
  XnLicense* licenses = 0;

  try {
    // XMLをファイルから設定情報を取得して初期化する
    std::cout << "xn::Context::InitFromXmlFile ... ";
    xn::Context context;
    XnStatus rc = context.InitFromXmlFile( CONFIG_XML_PATH );
    if ( rc != XN_STATUS_OK ) {
      throw std::runtime_error( ::xnGetStatusString( rc ) );
    }
    std::cout << "Success" << std::endl;

    // ライセンス情報を取得する
    std::cout << "xn::Context::EnumerateLicenses ... ";
    XnUInt32 lisenceCount = 0;
    rc = context.EnumerateLicenses( licenses, lisenceCount );
    if ( rc != XN_STATUS_OK ) {
      throw std::runtime_error( ::xnGetStatusString( rc ) );
    }
    std::cout << "Success" << std::endl;

    for ( int i = 0; i < lisenceCount; ++i ) {
      std::cout << licenses[i].strVendor << ", " <<
        licenses[i].strKey << std::endl;
    }


    // 登録されたデバイスを取得する
    std::cout << "xn::Context::EnumerateExistingNodes ... ";
    xn::NodeInfoList nodeList;
    rc = context.EnumerateExistingNodes( nodeList );
    if ( rc != XN_STATUS_OK ) {
      throw std::runtime_error( ::xnGetStatusString( rc ) );
    }
    std::cout << "Success" << std::endl;

    for ( xn::NodeInfoList::Iterator it = nodeList.Begin();
      it != nodeList.End(); ++it ) {
        std::cout <<
          ::xnProductionNodeTypeToString( (*it).GetDescription().Type ) <<
          ", " <<
          (*it).GetInstanceName() << ", " <<
          (*it).GetDescription().strName << ", " <<
          (*it).GetDescription().strVendor << ", " <<
          std::endl;
    }

    std::cout << "Shutdown" << std::endl;
  }
  catch ( std::exception& ex ) {
    std::cout << ex.what() << std::endl;
  }

  if ( licenses != 0 ) {
    xn::Context::FreeLicensesList( licenses );
  }

  return 0;
}
