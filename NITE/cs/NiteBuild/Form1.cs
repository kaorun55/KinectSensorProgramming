using System;
using System.Windows.Forms;

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
        xn.Context context = new xn.Context(CONFIG_XML_PATH);
        xnv.SessionManager sessionManager = new xnv.SessionManager(context,
                                                "Wave,Click", "RaiseHand");
        MessageBox.Show("Success");
      }
      catch (Exception ex) {
        MessageBox.Show(ex.Message);
      }
    }
  }
}
