#include "calculator.hpp"

void print_help() {
    std::cout << "========== 科学计算器命令行模式 ==========\n";
    std::cout << "命令格式: -command [参数]\n";
    std::cout << "可用命令:\n";
    std::cout << "  -calc <expression>                   计算表达式\n";
    std::cout << "  -infix <expression>                  显示中缀表达式解析结果\n";
    std::cout << "  -postfix <expression>                显示后缀表达式解析结果\n";
    std::cout << "  -valid <expression>               验证表达式语法\n";
    std::cout << "  -clear                               清空屏幕\n";
    std::cout << "  -help                                显示帮助\n";
    std::cout << "  -exit                                退出程序\n";
    std::cout << "支持的运算符和函数:\n";
    std::cout << "  算术: + - * / % ^ !\n";
    std::cout << "  函数: sin cos tan cot sec csc arcsin arccos arctan arccot arcsec arccsc\n";
    std::cout << "        ln lg sqrt cbrt deg rad\n";
    std::cout << "  常数: PI E PHI\n";
    std::cout << "  进制: 0b(二进制) 0o(八进制) 0x(十六进制)\n";
    std::cout << "示例:\n";
    std::cout << "  -calc \"2 + 3 * 4\"\n";
    std::cout << "  -calc \"sin(PI/2)\"\n";
    std::cout << "  -calc \"0b1010 + 0x1F\"\n";
    std::cout << "  -validate \"2 * (3 + 4)\"\n";
}

bool parse_command(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "错误: 缺少命令参数\n";
        print_help();
        return false;
    }

    std::string command = argv[1];

    if (command == "-help") {
        print_help();
        return true;
    }
    else if (command == "-clear") {
        system("cls");
        return true;
    }
    else if (command == "-exit") {
        std::cout << "感谢使用，再见!\n";
        return false;
    }
    else if (command == "-calc" || command == "-infix" || command == "-postfix" || command == "-validate") {
        if (argc < 3) {
            std::cout << "错误: 缺少表达式参数\n";
            std::cout << "用法: " << command << " <expression>\n";
            return false;
        }

        std::string expression = argv[2];

        try {
            if (command == "-calc") {
                chr::expression expr(expression);
                std::cout << "计算结果: " << expr.evaluate_from_infix() << "\n";
            }
            else if (command == "-infix") {
                chr::expression expr(expression);
                std::cout << "中缀解析: " << expr.infix_expression() << "\n";
            }
            else if (command == "-postfix") {
                chr::expression expr(expression);
                std::cout << "后缀解析: " << expr.postfix_expression() << "\n";
            }
            else if (command == "-valid") {
                chr::expression_tokenizer tokenizer;
                if (tokenizer.validate(expression)) {
                    std::cout << "表达式语法正确!\n";
                }
                else {
                    std::cout << "表达式语法错误!\n";
                    std::cout << tokenizer.detailed_analysis() << "\n";
                    return false;
                }
            }
        }
        catch (const std::exception& e) {
            std::cout << "错误: " << e.what() << std::endl;
            return false;
        }
    }
    else {
        std::cout << "错误: 未知命令: " << command << "\n";
        print_help();
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {
    // 如果有命令行参数，则解析并执行相应命令
    if (argc > 1) {
        if (!parse_command(argc, argv)) {
            return 1; // 如果命令执行失败，返回错误码
        }
        // 命令执行成功后，继续进入交互模式
        std::cout << "命令执行完成，进入交互模式...\n";
    }

    // 交互模式
    std::cout << "欢迎使用科学计算器!\n";
    std::cout << "输入 -help 查看可用命令\n";

    std::string input;
    while (true) {
        std::cout << "\n> ";
        std::getline(std::cin, input);

        if (input.empty()) continue;

        // 解析输入为命令行参数
        std::vector<std::string> args;
        std::istringstream iss(input);
        std::string token;

        while (iss >> token) {
            // 处理引号包围的参数
            if (token.front() == '"') {
                std::string quoted_arg = token;
                while (quoted_arg.back() != '"' && iss >> token) {
                    quoted_arg += " " + token;
                }
                if (quoted_arg.back() == '"') {
                    args.push_back(quoted_arg.substr(1, quoted_arg.length() - 2));
                }
                else {
                    args.push_back(quoted_arg.substr(1));
                }
            }
            else {
                args.push_back(token);
            }
        }

        if (args.empty()) continue;

        // 转换为char*数组格式
        std::vector<char*> cargs;
        cargs.push_back(argv[0]); // 程序名
        for (auto& arg : args) {
            cargs.push_back(&arg[0]);
        }

        if (!parse_command(cargs.size(), cargs.data())) {
            if (args[0] == "-exit") {
                break;
            }
        }
    }

    return 0;
}