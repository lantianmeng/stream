#include <Windows.h>
#include <boost/make_shared.hpp>

#include "ui_showRGB.h"
#include "fileDriver.h"
#include "streamDecode.h"
#include "streamHandler.h"
#include "RGBWnd.h"

//[��ȡ����̨���ڵľ��](https://blog.csdn.net/a199228/article/details/6696744)
typedef HWND (WINAPI *PROCGETCONSOLEWINDOW)();
PROCGETCONSOLEWINDOW GetConsoleWindowHwnd;

int main(int argc, char * argv[])
{
	if (argc < 2 )
	{
		return -1;
	}

	HMODULE hKernel32 = GetModuleHandle("kernel32");
	GetConsoleWindowHwnd = (PROCGETCONSOLEWINDOW)GetProcAddress(hKernel32,"GetConsoleWindow");

	boost::shared_ptr<CStreamDecode> decoder = boost::make_shared<CStreamDecode>();
	decoder->FFmpeg_H264DecoderInit();

	QApplication app(argc,argv);

	//QWidget *dialog = new QWidget;
	//boost::shared_ptr<Ui::Form> ui = boost::make_shared<Ui::Form>();
	//ui->setupUi(dialog);

	boost::shared_ptr<QGLCanvas> ui = boost::make_shared<QGLCanvas>();

	//boost::shared_ptr<RGBDataBuffer> data= boost::make_shared<RGBDataBuffer>(5, ui);

	boost::shared_ptr<CStreamHandler> handler = boost::make_shared<CStreamHandler>(decoder, ui);
	
	ui->show();

	CFileDriver h264File(argv[1], handler.get());

	//��h264�ļ������ţ��ļ�����֮�󣬲��Ž���
	h264File.init();
	h264File.readStreamData();

	//h264�ļ���Ϊ��ƵԴ������ϣ��ļ��������ͷ��ʼ��ȡ��
	//int ret;
	//do 
	//{
	//	if (!h264File.init())
	//	{
	//		std::cout << "h264File.init error" << std::endl;
	//		break;
	//	}

	//	ret = h264File.readStreamData();
	//	
	//} while (ret == AVERROR_EOF);

	//system("pause");
	//return 0;

	return app.exec();
	
}