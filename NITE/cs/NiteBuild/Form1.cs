using System;
using System.Windows.Forms;
using OpenNI;
using NITE;

namespace NiteBuild
{
  public partial class Form1 : Form
  {
    public Form1()
    {
      InitializeComponent();
    }

    private void Form1_Load(object sender, EventArgs e)
    {
      try {
        // 設定ファイルのパス(環境に合わせて変更してください)
        const string CONFIG_XML_PATH = @"../../../../../Data/SamplesConfig.xml";
        ScriptNode scriptNode;
        Context context = Context.CreateFromXmlFile( CONFIG_XML_PATH, out scriptNode );
        SessionManager sessionManager = new SessionManager( context,
                                                "Wave,Click", "RaiseHand");
        MessageBox.Show("Success");
      }
      catch (Exception ex) {
        MessageBox.Show(ex.Message);
      }
    }
  }
}
