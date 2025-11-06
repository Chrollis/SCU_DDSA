#include "calculator.hpp"

namespace chr {
	namespace {
		const std::string binary_pattern = R"((0b[01]+(\.[01]*)?))";
		const std::string octal_pattern = R"((0o[0-7]+(\.[0-7]*)?))";
		const std::string hexadecimal_pattern = R"((0x[0-9A-Fa-f]+(\.[0-9A-Fa-f]*)?))";
		const std::string decimal_pattern = R"(((\d+\.?\d*|\.\d+)([eE][-+]?\d+)?))";
		const std::string constant_pattern = R"(PI|E|PHI)";
		const std::string normal_pattern = R"([+\-*/^()!%])";
		const std::string signal_pattern = R"(pos|neg)";
		const std::string function_pattern =
			R"(sin|cos|tan|cot|sec|csc|)"
			R"(arcsin|arccos|arctan|arccot|arcsec|arccsc|)"
			R"(ln|lg|deg|rad|sqrt|cbrt)";
	}
	byte operator&(token_t a, token_t b) noexcept
	{
		return static_cast<byte>(a) & static_cast<byte>(b);
	}
	token_t token_type(const std::string& str) noexcept {
		if (std::regex_match(str, std::regex(binary_pattern))) {
			return token_t::binary_number;
		}
		else if (std::regex_match(str, std::regex(octal_pattern))) {
			return token_t::octal_number;
		}
		else if (std::regex_match(str, std::regex(hexadecimal_pattern))) {
			return token_t::hexadecimal_number;
		}
		else if (std::regex_match(str, std::regex(decimal_pattern))) {
			return token_t::decimal_number;
		}
		else if (std::regex_match(str, std::regex(normal_pattern))) {
			return token_t::normal_operator;
		}
		else if (std::regex_match(str, std::regex(constant_pattern))) {
			return token_t::constant_number;
		}
		else if (std::regex_match(str, std::regex(function_pattern))) {
			return token_t::function_operator;
		}
		else if (std::regex_match(str, std::regex(signal_pattern))) {
			return token_t::signal_operator;
		}
		return token_t::invalid_token;
	}
	bool expression_tokenizer::tokenize(const std::string& expression) {
		m_tokens.clear();
		m_errors.clear();
		std::regex pattern(binary_pattern + "|" + octal_pattern + "|" + hexadecimal_pattern + "|" +
			decimal_pattern + "|" + constant_pattern + "|" + normal_pattern + "|" + function_pattern);
		size_t pos = 0;
		auto words_begin = std::sregex_iterator(expression.begin(), expression.end(), pattern);
		auto words_end = std::sregex_iterator();
		for (auto& it = words_begin; it != words_end; it++) {
			std::smatch match = *it;
			std::string token = match.str();
			size_t token_pos = match.position();
			if (std::all_of(token.begin(), token.end(), isspace)) {
				continue;
			}
			if (token_pos > pos) {
				std::string unknown = expression.substr(pos, token_pos - pos);
				if (!std::all_of(unknown.begin(), unknown.end(), isspace)) {
					m_errors.push_back({ unknown,"无法识别的字符或符号" });
				}
			}
			m_tokens.push_back(token);
			pos = token_pos + token.length();
		}
		if (pos < expression.length()) {
			std::string remaining = expression.substr(pos);
			if (!std::all_of(remaining.begin(), remaining.end(), isspace)) {
				m_errors.push_back({ remaining, "表达式末尾有无法识别的字符" });
			}
		}
		parse_signal_operators();
		return m_errors.empty();
	}
	bool expression_tokenizer::validate(const std::string& expression) {
		if (!tokenize(expression)) {
			return 0;
		}
		parse_parenthese();
		parse_operator_sequence();
		parse_number_format();
		parse_function_usage();
		return m_errors.empty();
	}
	void expression_tokenizer::parse_signal_operators() {
		std::vector<std::string> processed_tokens;
		for (size_t i = 0; i < m_tokens.size(); ++i) {
			const std::string& token = m_tokens[i];
			if (token == "+" || token == "-") {
				if (i == 0 || (i > 0 && (is_operator(m_tokens[i - 1]) && m_tokens[i - 1] != ")"
					&& m_tokens[i - 1] != "!") || is_function(m_tokens[i - 1]))) {
					processed_tokens.push_back(token == "+" ? "pos" : "neg");
				}
				else {
					processed_tokens.push_back(token);
				}
			}
			else {
				processed_tokens.push_back(token);
			}
		}
		m_tokens = processed_tokens;
	}
	void expression_tokenizer::parse_parenthese() {
		std::stack<std::pair<std::string, size_t>>paren_stack;
		for (size_t i = 0; i < m_tokens.size(); i++) {
			const auto& token = m_tokens[i];
			if (token == "(") {
				paren_stack.push({ token,i });
			}
			else if (token == ")") {
				if (paren_stack.empty()) {
					add_error(std::to_string(i), "存在多余的右括弧");
				}
				else {
					paren_stack.pop();
				}
			}
		}
		while (!paren_stack.empty()) {
			add_error(std::to_string(paren_stack.top().second), "存在多余的左括弧");
			paren_stack.pop();
		}
	}
	void expression_tokenizer::parse_operator_sequence() {
		for (size_t i = 0; i < m_tokens.size(); ++i) {
			const std::string& token = m_tokens[i];
			if (token_type(token) == token_t::signal_operator) {
				if (i == m_tokens.size() - 1) {
					add_error(std::to_string(i), "表达式以运算符结尾");
				}
				else {
					if (i != 0 && token_type(m_tokens[i - 1]) == token_t::signal_operator) {
						add_error(std::to_string(i), "表达式含有连续符号运算符");
					}
				}
			}
			else if (token == "!") {
				if (i == 0) {
					add_error(std::to_string(i), "表达式以阶乘运算符开头");
				}
				else {
					const std::string& prev = m_tokens[i - 1];
					if (!(is_number(prev) || prev == ")")) {
						add_error(std::to_string(i), "阶乘运算符前面必须是数字、常量或表达式");
					}
				}
			}
			else if (token != "(" && token != ")" && token_type(token) == token_t::normal_operator) {
				if (i == 0) {
					add_error(std::to_string(i), "表达式以二元运算符开头");
				}
				else if (i == m_tokens.size() - 1) {
					add_error(std::to_string(i), "表达式以运算符结尾");
				}
				else if (token_type(m_tokens[i - 1]) == token_t::signal_operator) {
					add_error(std::to_string(i), "表达式含有连续二元运算符");
				}
			}
		}
	}
	void expression_tokenizer::parse_number_format() {
		for (size_t i = 0; i < m_tokens.size(); i++) {
			const auto& token = m_tokens[i];
			if (is_number(token) && !is_constant(token)) {
				if (i > 0 && is_number(m_tokens[i - 1])) {
					add_error(m_tokens[i - 1] + token, "表达式含有连续数字");
				}
				else {
					if ((token.find('e') != std::string::npos || token.find('E') != std::string::npos) &&
						!token.starts_with("0x") && !token.starts_with("0o") && !token.starts_with("0b")) {
						if (!std::regex_match(token, std::regex(decimal_pattern))) {
							add_error(token, "科学计数法格式错误");
						}
					}
					if (token.starts_with("0b") &&
						!std::regex_match(token, std::regex(binary_pattern))) {
						add_error(token, "二进制格式错误");
					}
					else if (token.starts_with("0o") &&
						!std::regex_match(token, std::regex(octal_pattern))) {
						add_error(token, "八进制格式错误");
					}
					else if (token.starts_with("0x") &&
						!std::regex_match(token, std::regex(hexadecimal_pattern))) {
						add_error(token, "十六进制格式错误");
					}
				}
			}
		}
	}
	void expression_tokenizer::parse_function_usage() {
		for (size_t i = 0; i < m_tokens.size(); ++i) {
			if (is_function(m_tokens[i]) && (i + 1 >= m_tokens.size() || m_tokens[i + 1] != "(")) {
				add_error(m_tokens[i], "函数名未紧跟左括号");
			}
		}
	}
	void expression_tokenizer::add_error(const std::string& position, const std::string& description) {
		m_errors.push_back({ position,description });
	}
	std::string expression_tokenizer::detailed_analysis() const {
		std::string str;
		for (const auto& token : m_tokens) {
			str += "【" + std::to_string(static_cast<byte>(token_type(token))) + "】：" + token + "\n";
		}
		for (const auto& error : m_errors) {
			str += "【" + error.first + "】：" + error.second + "\n";
		}
		str.pop_back();
		return str;
	}
	token token::from_string(const std::string& str) {
		std::string cpstr = str;
		while (!cpstr.empty() && std::isspace(cpstr.front())) {
			cpstr.erase(0, 1);
		}
		while (!cpstr.empty() && std::isspace(cpstr.back())) {
			cpstr.pop_back();
		}
		if (cpstr.empty()) {
			throw std::runtime_error("出现空白令牌");
		}
		if (auto number = try_parse_number(cpstr)) {
			return token::from_number(*number);
		}
		if (auto op_token = try_parse_operator(cpstr)) {
			return *op_token;
		}
		throw std::runtime_error("解析令牌出错");
	}
	inline std::optional<double> token::try_parse_number(const std::string& str) {
		if (!chr::is_number(str)) {
			return std::nullopt;
		}
		token_t type = token_type(str);
		if (type == token_t::decimal_number) {
			return std::stod(str);
		}
		else if (type == token_t::constant_number) {
			if (str == "E") {
				return CONSTANT_E;
			}
			else if (str == "PI") {
				return CONSTANT_PI;
			}
			else if (str == "PHI") {
				return CONSTANT_PHI;
			}
			else {
				throw std::runtime_error("出现无效常数");
			}
		}
		else {
			double value = 0;
			int radix = 10;
			std::string integer;
			std::string fraction;
			if (type == token_t::binary_number) {
				radix = 2;
			}
			else if (type == token_t::octal_number) {
				radix = 8;
			}
			else if (type == token_t::hexadecimal_number) {
				radix = 16;
			}
			else {
				throw std::runtime_error("出现无效进制");
			}
			size_t dot_pos = str.find('.');
			if (dot_pos == std::string::npos) {
				integer = str.substr(2);
			}
			else {
				integer = str.substr(2, dot_pos - 2);
				fraction = str.substr(dot_pos + 1);
			}
			for (size_t i = 0; i < integer.length(); i++) {
				char t = integer[integer.length() - i - 1];
				value += pow(radix, i) * (t <= '9' ? t - '0' : t <= 'Z' ? t - 'A' + 10 : t - 'a' + 10);
			}
			for (size_t i = 0; i < fraction.length(); i++) {
				char t = fraction[i];
				value += pow(radix, -(int(i) + 1)) * (t <= '9' ? t - '0' : t <= 'Z' ? t - 'A' + 10 : t - 'a' + 10);
			}
			return value;
		}
	}
	std::optional<token> token::try_parse_operator(const std::string& str) {
		static const std::unordered_map<std::string, token(*)(void)> operator_map = {
			{"+", []() { return token::add(); }},
			{"-", []() { return token::minus(); }},
			{"*", []() { return token::multiply(); }},
			{"/", []() { return token::divide(); }},
			{"%", []() { return token::modulo(); }},
			{"^", []() { return token::exponent(); }},
			{"!", []() { return token::factorial(); }},
			{"(", []() { return token::left_parentheses(); }},
			{")", []() { return token::right_parentheses(); }},
			{"sin", []() { return token::sine(); }},
			{"cos", []() { return token::cosine(); }},
			{"tan", []() { return token::tangent(); }},
			{"cot", []() { return token::cotangent(); }},
			{"sec", []() { return token::secant(); }},
			{"csc", []() { return token::cosecant(); }},
			{"arcsin", []() { return token::arcsine(); }},
			{"arccos", []() { return token::arccosine(); }},
			{"arctan", []() { return token::arctangent(); }},
			{"arccot", []() { return token::arccotangent(); }},
			{"arcsec", []() { return token::arcsecant(); }},
			{"arccsc", []() { return token::arccosecant(); }},
			{"lg", []() { return token::common_logarithm(); }},
			{"ln", []() { return token::natural_logarithm(); }},
			{"sqrt", []() { return token::square_root(); }},
			{"cbrt", []() { return token::cubic_root(); }},
			{"deg", []() { return token::degree(); }},
			{"rad", []() { return token::radian(); }},
		};
		auto it = operator_map.find(str);
		if (it != operator_map.end()) {
			return it->second();
		}
		return std::nullopt;
	}
	void expression::calculate(std::stack<token>& operands, const token& op) const {
		byte operand_num = op.operator_operand_num();
		if (operand_num == 0) {
			throw std::runtime_error("计算时出现零操作数运算符");
		}
		else if (operand_num == 1) {
			double a = operands.top().number_value();
			operands.pop();
			operands.push(token::from_number(op.apply_operator(a, 0)));
		}
		else if (operand_num == 2) {
			double b = operands.top().number_value();
			operands.pop();
			double a = operands.top().number_value();
			operands.pop();
			operands.push(token::from_number(op.apply_operator(a, b)));
		}
		else {
			throw std::runtime_error("计算时出现操作数多于两个的运算符");
		}
	}
	expression::expression(const std::string& infix_expression) {
		expression_tokenizer tokenizer;
		if (!tokenizer.validate(infix_expression)) {
			throw std::runtime_error("表达式非法：\n" + tokenizer.detailed_analysis());
		}
		std::vector<std::string> strings = tokenizer.tokens();
		for (const auto& str : strings) {
			m_infix.push_back(token::from_string(str));
		}
		std::stack<token> ops;
		for (const auto& tk : m_infix) {
			token_t type = tk.type();
			if (type == token_t::number_token) {
				m_postfix.push_back(tk);
			}
			else {
				if (tk.operator_symbol() == "(") {
					ops.push(tk);
				}
				else if (tk.operator_symbol() == ")") {
					while (!ops.empty()) {
						if (ops.top().operator_symbol() == "(") {
							ops.pop();
							break;
						}
						else {
							m_postfix.push_back(ops.top());
							ops.pop();
						}
					}
				}
				else {
					while (!ops.empty() && ops.top().operator_prioriry() >= tk.operator_prioriry()) {
						m_postfix.push_back(ops.top());
						ops.pop();
					}
					ops.push(tk);
				}
			}
		}
		while (!ops.empty()) {
			m_postfix.push_back(ops.top());
			ops.pop();
		}
	}
	std::string expression::infix_expression() const {
		std::string str;
		for (const auto& tk : m_infix) {
			if (tk.type() == token_t::number_token) {
				str += std::to_string(tk.number_value()) + ' ';
			}
			else {
				str += tk.operator_symbol() + ' ';
			}
		}
		return str;
	}
	std::string expression::postfix_expression() const {
		std::string str;
		for (const auto& tk : m_postfix) {
			if (tk.type() == token_t::number_token) {
				str += std::to_string(tk.number_value()) + ' ';
			}
			else {
				str += tk.operator_symbol() + ' ';
			}
		}
		return str;
	}
	double expression::evaluate_from_postfix() const {
		std::stack<token> operands;
		for (const auto& tk : m_postfix) {
			if (tk.type() == token_t::number_token) {
				operands.push(tk);
			}
			else {
				calculate(operands, tk);
			}
		}
		if (operands.size() != 1) {
			throw std::runtime_error("运算结束时出错，操作数栈不只有一个元素");
		}
		return operands.top().number_value();
	}
	double expression::evaluate_from_infix() const {
		std::stack<token> operands;
		std::stack<token> ops;
		for (const auto& tk : m_infix) {
			if (tk.type() == token_t::number_token) {
				operands.push(tk);
			}
			else {
				if (tk.operator_symbol() == "(") {
					ops.push(tk);
				}
				else if (tk.operator_symbol() == ")") {
					while (!ops.empty()) {
						if (ops.top().operator_symbol() == "(") {
							ops.pop();
							break;
						}
						else {
							calculate(operands, ops.top());
							ops.pop();
						}
					}
				}
				else {
					while (!ops.empty() && ops.top().operator_prioriry() >= tk.operator_prioriry()) {
						calculate(operands, ops.top());
						ops.pop();
					}
					ops.push(tk);
				}
			}
		}
		while (!ops.empty()) {
			calculate(operands, ops.top());
			ops.pop();
		}
		if (operands.size() != 1) {
			throw std::runtime_error("运算结束时出错，操作数栈不只有一个元素");
		}
		return operands.top().number_value();
	}
}