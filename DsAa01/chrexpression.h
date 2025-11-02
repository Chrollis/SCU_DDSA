#ifndef CHREXPRESSION_H
#define CHREXPRESSION_H

#include "chrtoken.h"
#include "chrvalidator.h"
#include <sstream>

namespace chr {
	class basic_expression {
	protected:
		std::vector<basic_token*> _content;
		static void calculate(std::stack<number_token>& operand_stack, const operator_token& op);
	public:
		std::vector<basic_token*> content()const { return _content; }
		virtual ~basic_expression();
		virtual long double evaluate()const = 0;
	};

	std::ostream& operator<<(std::ostream& os, const basic_expression& expr);

	class infix_expression :public basic_expression {
	public:
		infix_expression(const std::string& infix_expr_str);
		long double evaluate()const;
	};

	class postfix_expression :public basic_expression {
	public:
		postfix_expression(const std::string& infix_expr_str);
		long double evaluate()const;
	};
};

#endif
