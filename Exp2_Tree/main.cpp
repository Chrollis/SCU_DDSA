#include "compressor.hpp"

// 程序入口：命令行交互式解析并调用压缩/解压函数
int main() {
    std::string str;
    bool is_compress = 0;
    while (1) {
        try {
            // 显示使用帮助提示
            std::cout << "使用方法：\n"
                "-c 压缩\t-d 解压缩（二选一）\n"
                "-s [源文件地址] 必填，读取源文件（若包含引号需加括号，解压缩要求为.huff后缀文件）\n"
                "-dd [目标文件夹] 默认源文件所在文件夹（若包含引号需加括号）\n"
                "-dn [输出文件名] 默认源文件名，包含文件后缀（若包含引号需加括号，默认下压缩将会额外添加.huff后缀，解压缩时会去掉.huff后缀）\n"
                "-o <选项> 显示细节，选项包含：【1，显示详细压缩率；2，以树型结构显示Haffman树；3，均显示】（解压缩时无法显示压缩细节）\n"
                "-ex 退出程序（以上顺序任意，不合法将会报错）\n\n"
                "# 例：-c -s \"C:\\Users\\Administrator\\Desktop\\test.txt\" -dd \"C:\\Users\\Administrator\\Downloads\" -dn test.huff -o 2\n"
                "# 将执行对桌面文件test.txt的压缩，并将压缩文件命名为test.huff放在下载目录下，同时显示Huffman树\n\n"
                "输入指令：";
            std::getline(std::cin, str);
            chr::parse_command(str);
        }
        catch (std::runtime_error& e) {
            // 捕获并显示解析或运行时产生的错误信息
            std::cout << e.what() << std::endl;
        }
        // 等待用户按键并清屏，准备下一次交互
        system("pause");
        system("cls");
    }
    return 0;
}