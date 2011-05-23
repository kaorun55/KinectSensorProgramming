// C#版は今回のみ「コンソール アプリケーション」で作成しています。
using System;

namespace Context
{
    class Program
    {
        static void Main(string[] args)
        {
            try {
                // 設定ファイルのパス(環境に合わせて変更してください)
                string CONFIG_XML_PATH = @"../../../../../Data/SamplesConfig.xml";

                // XMLをファイルから設定情報を取得して初期化する
                Console.Write(@"xn.Context.InitFromXmlFile ... ");
                xn.Context context = new xn.Context(CONFIG_XML_PATH);
                Console.WriteLine(@"Success");

                // ライセンス情報を取得する
                Console.Write(@"xn.Context.EnumerateLicenses ... ");
                xn.License[] licenses = context.EnumerateLicenses();
                Console.WriteLine(@"Success");

                foreach (xn.License license in licenses) {
                    Console.WriteLine(license.strVendor + @", " + license.strKey);
                }

                // 登録されたデバイスを取得する
                Console.Write(@"xn.Context.EnumerateExistingNodes ... ");
                xn.NodeInfoList nodeList = context.EnumerateExistingNodes();
                Console.WriteLine(@"Success");

                foreach (xn.NodeInfo node in nodeList) {
                    // GetDescriptionの呼び出しで落ちる、、、
                    //Console.WriteLine(node.GetDescription().strName + "," +
                    //                  node.GetDescription().strVendor + "," +
                    //                  node.GetInstanceName() + ",");
                    Console.WriteLine(node.GetInstanceName());
                }

                Console.WriteLine(@"Shutdown");
            }
            catch (Exception ex) {
                Console.WriteLine(ex.Message);
            }
        }
    }
}
