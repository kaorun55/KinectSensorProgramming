using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MultiProcessFlowServer
{
    class Program
    {
        static void Main(string[] args)
        {
            // 設定ファイルのパス(環境に合わせて変更してください)
            const string CONFIG_XML_PATH = @"../../../../../Data/SamplesConfig.xml";

            // コンテキスト、セッションマネージャー、マルチプロセスサーバの作成
            xn.Context context = new xn.Context(CONFIG_XML_PATH);

            xnv.SessionManager sessionManager = new xnv.SessionManager(context,
                                                    "Wave,Click", "RaiseHand");
            
        }
    }
}
