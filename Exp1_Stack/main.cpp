#include "calculator.hpp"

int main()
{
    std::string str;
    while (1) {
        try
        {
            std::cout << "输入表达式：";
            std::getline(std::cin, str);
            if (str == "exit") {
                return 0;
            }
            else if (str == "clear") {
                system("cls");
            }
            else {
                chr::expression expr(str);
                std::cout << "中缀解析：" << expr.infix_expression() << "\n";
                std::cout << "后缀解析：" << expr.postfix_expression() << "\n";
                std::cout << "中缀计算：" << expr.evaluate_from_infix() << "\n";
                std::cout << "后缀计算：" << expr.evaluate_from_postfix() << "\n";
            }
        }
        catch (std::runtime_error& e)
        {
            std::cout << e.what() << std::endl;
        }
    }
    return 0;
}