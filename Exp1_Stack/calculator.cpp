#include "calculator.hpp"

namespace chr {
	namespace {
		// 各种字面量与运算符的正则表达式模式
		// 二进制：0b1010 或 0b11.01（允许小数点）
		const std::string binary_pattern = R"((0b[01]+(\.[01]*)?))";
		// 八进制：0o755 或 0o7.3
		const std::string octal_pattern = R"((0o[0-7]+(\.[0-7]*)?))";
		// 十六进制：0x1A3F 或 0x1A.F
		const std::string hexadecimal_pattern = R"((0x[0-9A-Fa-f]+(\.[0-9A-Fa-f]*)?))";
		// 十进制（含小数与科学计数法）
		const std::string decimal_pattern = R"(((\d+\.?\d*|\.\d+)([eE][-+]?\d+)?))";
		// 支持的常数名
		const std::string constant_pattern = R"(PI|E|PHI)";
		// 普通运算符与括弧、阶乘、取余等单字符符号
		const std::string normal_pattern = R"([+\-*/^()!%])";
		// 内部使用的一元符号替代（pos/neg）
		const std::string signal_pattern = R"(pos|neg)";
		// 支持的函数列表（匹配单词）
		const std::string function_pattern =
			R"(sin|cos|tan|cot|sec|csc|)"
			R"(arcsin|arccos|arctan|arccot|arcsec|arccsc|)"
			R"(ln|lg|deg|rad|sqrt|cbrt)";
	}

	// 用于位掩码判断 token_t 的按位与实现（用于 is_number / is_operator 等）
	byte operator&(token_t a, token_t b) noexcept
	{
		return static_cast<byte>(a) & static_cast<byte>(b);
	}

	// 根据字符串内容判断其 token 类型（使用正则按优先级匹配）
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

	// 分词：使用正则迭代器匹配已知 token 模式，同时捕获未知字符作为错误
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
			// 忽略全是空白的匹配（虽然正则通常不会匹配空白）
			if (std::all_of(token.begin(), token.end(), isspace)) {
				continue;
			}
			// 如果当前匹配之前存在未匹配的内容，视为未知字符或错误
			if (token_pos > pos) {
				std::string unknown = expression.substr(pos, token_pos - pos);
				if (!std::all_of(unknown.begin(), unknown.end(), isspace)) {
					m_errors.push_back({ unknown,"无法识别的字符或符号" });
				}
			}
			m_tokens.push_back(token);
			pos = token_pos + token.length();
		}
		// 检查末尾是否有剩余无法识别内容
		if (pos < expression.length()) {
			std::string remaining = expression.substr(pos);
			if (!std::all_of(remaining.begin(), remaining.end(), isspace)) {
				m_errors.push_back({ remaining, "表达式末尾有无法识别的字符" });
			}
		}
		// 将一元 + / - 解析为内部标记 pos/neg，方便后续验证与计算
		parse_signal_operators();
		return m_errors.empty();
	}

	// 完整验证：先分词，后逐项语法检查
	bool expression_tokenizer::validate(const std::string& expression) {
		if (!tokenize(expression)) {
			return 0;
		}
		parse_parenthese();         // 括弧配对检查
		parse_operator_sequence();  // 运算符顺序合法性检查
		parse_number_format();      // 数字字面量格式检查
		parse_function_usage();     // 函数调用形式检查（函数名后必须跟 '('）
		return m_errors.empty();
	}

	// 把作为二元运算符的 + - 在合适位置识别为一元运算符 pos/neg
	void expression_tokenizer::parse_signal_operators() {
		std::vector<std::string> processed_tokens;
		for (size_t i = 0; i < m_tokens.size(); ++i) {
			const std::string& token = m_tokens[i];
			if (token == "+" || token == "-") {
				// 如果是表达式开头，或前一个是运算符(且不是右括弧或阶乘)或函数名，视为一元符号
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

	// 检查括弧匹配，并记录多余左右括弧的错误位置
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
		// 剩余左括弧视为多余
		while (!paren_stack.empty()) {
			add_error(std::to_string(paren_stack.top().second), "存在多余的左括弧");
			paren_stack.pop();
		}
	}

	// 检查运算符序列的合法性（连续运算符 / 运算符起始或结尾等）
	void expression_tokenizer::parse_operator_sequence() {
		for (size_t i = 0; i < m_tokens.size(); ++i) {
			const std::string& token = m_tokens[i];
			// 一元符号（pos/neg）不能出现在表达式末尾，也不能连续出现
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
			// 阶乘必须跟在数字/常量或右括弧后面
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
			// 普通二元运算符（除了括弧）检查起始/结尾与连续二元运算符
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

	// 数字字面量格式检查：进制、科学计数法、连续数字等
	void expression_tokenizer::parse_number_format() {
		for (size_t i = 0; i < m_tokens.size(); i++) {
			const auto& token = m_tokens[i];
			// 仅对被识别为数字且不是常量的 token 进行格式检查
			if (is_number(token) && !is_constant(token)) {
				// 上一个也是数字 -> 连续数字错误
				if (i > 0 && is_number(m_tokens[i - 1])) {
					add_error(m_tokens[i - 1] + token, "表达式含有连续数字");
				}
				else {
					// 科学计数法校验：确保符合 decimal_pattern（且不能是 0x/0o/0b 前缀）
					if ((token.find('e') != std::string::npos || token.find('E') != std::string::npos) &&
						!token.starts_with("0x") && !token.starts_with("0o") && !token.starts_with("0b")) {
						if (!std::regex_match(token, std::regex(decimal_pattern))) {
							add_error(token, "科学计数法格式错误");
						}
					}
					// 各进制字面量格式校验
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

	// 函数使用检查：函数名后必须紧接 '('，否则报错
	void expression_tokenizer::parse_function_usage() {
		for (size_t i = 0; i < m_tokens.size(); ++i) {
			if (is_function(m_tokens[i]) && (i + 1 >= m_tokens.size() || m_tokens[i + 1] != "(")) {
				add_error(m_tokens[i], "函数名未紧跟左括号");
			}
		}
	}

	// 添加错误记录（位置与描述）
	void expression_tokenizer::add_error(const std::string& position, const std::string& description) {
		m_errors.push_back({ position,description });
	}

	// 返回详细分析字符串：列出每个 token 的类型数字值与 token 字符串，再列出错误
	std::string expression_tokenizer::detailed_analysis() const {
		std::string str;
		for (const auto& token : m_tokens) {
			str += "【" + std::to_string(static_cast<byte>(token_type(token))) + "】：" + token + "\n";
		}
		for (const auto& error : m_errors) {
			str += "【" + error.first + "】：" + error.second + "\n";
		}
		// 如果没有任何内容 pop_back 可能 UB，但在此上下文通常有内容；保持与原实现一致
		str.pop_back();
		return str;
	}

	// 将字符串解析为 token（数字或操作符），不含空白
	token token::from_string(const std::string& str) {
		std::string cpstr = str;
		// 去除前后空白
		while (!cpstr.empty() && std::isspace(cpstr.front())) {
			cpstr.erase(0, 1);
		}
		while (!cpstr.empty() && std::isspace(cpstr.back())) {
			cpstr.pop_back();
		}
		if (cpstr.empty()) {
			throw std::runtime_error("出现空白令牌");
		}
		// 尝试解析为数字
		if (auto number = try_parse_number(cpstr)) {
			return token::from_number(*number);
		}
		// 尝试解析为操作符/函数
		if (auto op_token = try_parse_operator(cpstr)) {
			return *op_token;
		}
		throw std::runtime_error("解析令牌出错");
	}

	// 尝试将字符串转换为数字值（支持常数与多进制字面量）
	inline std::optional<double> token::try_parse_number(const std::string& str) {
		if (!chr::is_number(str)) {
			return std::nullopt;
		}
		token_t type = token_type(str);
		// 十进制直接用 stod（支持科学计数法）
		if (type == token_t::decimal_number) {
			return std::stod(str);
		}
		// 常数替换为数值
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
		// 处理二/八/十六进制字面量（解析整数与小数部分）
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
				// 去掉前缀（0b/0o/0x）
				integer = str.substr(2);
			}
			else {
				integer = str.substr(2, dot_pos - 2);
				fraction = str.substr(dot_pos + 1);
			}
			// 解析整数部分（从低位到高位）
			for (size_t i = 0; i < integer.length(); i++) {
				char t = integer[integer.length() - i - 1];
				value += pow(radix, i) * (t <= '9' ? t - '0' : t <= 'Z' ? t - 'A' + 10 : t - 'a' + 10);
			}
			// 解析小数部分
			for (size_t i = 0; i < fraction.length(); i++) {
				char t = fraction[i];
				value += pow(radix, -(int(i) + 1)) * (t <= '9' ? t - '0' : t <= 'Z' ? t - 'A' + 10 : t - 'a' + 10);
			}
			return value;
		}
	}

	// 将操作符字符串映射为 token（通过工厂方法）
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

	// 根据操作符的 operand_num 执行相应的出栈计算并将结果压回
	void expression::calculate(std::stack<token>& operands, const token& op) const {
		byte operand_num = op.operator_operand_num();
		if (operand_num == 0) {
			throw std::runtime_error("计算时出现零操作数运算符");
		}
		else if (operand_num == 1) {
			// 一元运算，从栈顶取一个
			double a = operands.top().number_value();
			operands.pop();
			operands.push(token::from_number(op.apply_operator(a, 0)));
		}
		else if (operand_num == 2) {
			// 二元运算，注意栈顺序：先弹出 b，再弹出 a
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

	// 构造函数：验证表达式 -> 将字符串 token 转为 token 对象 -> 中缀转后缀（Shunting-yard）
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
				// 数字直接加入后缀表达式
				m_postfix.push_back(tk);
			}
			else {
				// 左括弧入栈
				if (tk.operator_symbol() == "(") {
					ops.push(tk);
				}
				// 右括弧弹出至对应左括弧
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
				// 普通运算符：依据优先级弹出栈顶更高或等于优先级的操作符
				else {
					while (!ops.empty() && ops.top().operator_prioriry() >= tk.operator_prioriry()) {
						m_postfix.push_back(ops.top());
						ops.pop();
					}
					ops.push(tk);
				}
			}
		}
		// 将剩余操作符加入后缀
		while (!ops.empty()) {
			m_postfix.push_back(ops.top());
			ops.pop();
		}
	}

	// 将中缀 token 列表序列化为可读字符串（数字输出其数值，操作符输出符号文本）
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

	// 将后缀 token 列表序列化为字符串（便于调试）
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

	// 从后缀直接计算（用 calculate 辅助）
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

	// 直接按中缀计算（即时处理运算符优先级）
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
		// 处理剩余操作符
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