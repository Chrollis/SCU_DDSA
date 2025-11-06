#ifndef CALCULATOR_HPP
#define CALCULATOR_HPP

#include <iostream>
#include <string>
#include <regex>
#include <vector>
#include <stack>
#include <algorithm>
#include <unordered_map>
#include <cmath>
#include <stdexcept>
#include <functional>
#include <sstream>
#include <variant>
#include <optional>

namespace chr {

	using byte = unsigned char;
	enum class token_t : byte {
		invalid_token = 0x00,
		number_token = 0x10,
		constant_number,
		binary_number,
		octal_number,
		hexadecimal_number,
		decimal_number,
		operator_token = 0x20,
		signal_operator,
		normal_operator,
		function_operator
	};
	byte operator&(token_t a, token_t b) noexcept;
	token_t token_type(const std::string& str) noexcept;
	inline bool is_operator(const std::string& str) noexcept {
		return token_t::operator_token & token_type(str);
	}
	inline bool is_function(const std::string& str) noexcept {
		return token_t::function_operator == token_type(str);
	}
	inline bool is_constant(const std::string& str) noexcept {
		return token_t::constant_number == token_type(str);
	}
	inline bool is_number(const std::string& str) noexcept {
		return token_t::number_token & token_type(str);
	}
	class expression_tokenizer {
	private:
		std::vector<std::string> m_tokens;
		std::vector<std::pair<std::string, std::string>> m_errors;
	private:
		void parse_signal_operators();
		void parse_parenthese();
		void parse_operator_sequence();
		void parse_number_format();
		void parse_function_usage();
		void add_error(const std::string& position, const std::string& description);
	public:
		bool tokenize(const std::string& expression);
		bool validate(const std::string& expression);
		const std::vector<std::string>& tokens() const { return m_tokens; }
		const std::vector<std::pair<std::string, std::string>>& errors() const { return m_errors; }
		std::string detailed_analysis() const;
	};
	constexpr double CONSTANT_E = 2.718281828459;
	constexpr double CONSTANT_PI = 3.1415926535898;
	constexpr double CONSTANT_PHI = 0.61803398875;
	constexpr byte PRIORITY_FUNCTION = 0xFF;
	struct number_data {
		double value;
		number_data(double val = 0.0) :value(val) {}
	};
	struct operator_data {
		std::string symbol;
		byte operand_num;
		byte priority;
		std::function<double(double, double)> apply;

		operator_data(const std::string& sym = "", byte op_num = 0, byte pri = 0,
			std::function<double(double, double)>func = nullptr)
			: symbol(sym), operand_num(op_num), priority(pri), apply(std::move(func)) {}
	};
	class token {
		token_t m_type;
		std::variant<number_data, operator_data> m_data;
	public:
		token() :m_type(token_t::invalid_token), m_data() {}
		token(double val) :m_type(token_t::number_token), m_data(number_data{ val }) {}
		token(const std::string& sym, byte op_num, byte pri,
			std::function<double(double, double)>func)
			:m_type(token_t::operator_token), m_data(operator_data{ sym,op_num,pri,std::move(func) }) {}
		token_t type() const { return m_type; }
		bool is_number() const { return m_type == token_t::number_token; }
		bool is_operator() const { return m_type == token_t::operator_token; }
		bool is_valid() const { return m_type != token_t::invalid_token; }
		double number_value() const {
			return std::get<number_data>(m_data).value;
		}
		const std::string& operator_symbol() const {
			return std::get<operator_data>(m_data).symbol;
		}
		byte operator_operand_num() const {
			return std::get<operator_data>(m_data).operand_num;
		}
		byte operator_prioriry() const {
			return std::get<operator_data>(m_data).priority;
		}
		double apply_operator(double a, double b) const {
			return std::get<operator_data>(m_data).apply(a, b);
		}
		template <typename Visitor>
		auto visit(Visitor&& vis) -> decltype(auto) {
			return std::visit(std::forward<Visitor>(vis), m_data);
		}
	public:
		static token from_number(double val) {
			return token(val);
		}
		static token add() {
			return token("+", 2, 1, [](double a, double b) {return a + b; });
		}
		static token minus() {
			return token("-", 2, 1, [](double a, double b) {return a - b; });
		}
		static token modulo() {
			return token("%", 2, 2, [](double a, double b) { return fmodl(a, b); });
		}
		static token multiply() {
			return token("*", 2, 3, [](double a, double b) {return a * b; });
		}
		static token divide() {
			return token("/", 2, 3, [](double a, double b) {return a / b; });
		}
		static token posite() {
			return token("pos", 1, 4, [](double a, double b) {return a; });
		}
		static token negate() {
			return token("neg", 1, 4, [](double a, double b) {return -a; });
		}
		static token exponent() {
			return token("^", 2, 5, [](double a, double b) {return pow(a, b); });
		}
		static token left_parentheses() {
			return token("(", 0, 0, [](double a, double b) {return 0; });
		}
		static token right_parentheses() {
			return token(")", 0, 0, [](double a, double b) {return 0; });
		}
		static token factorial() {
			return token("!", 1, 6, [](double a, double b) {return tgamma(a + 1); });
		}
		static token sine() {
			return token("sin", 1, PRIORITY_FUNCTION, [](double a, double b) {return sin(a); });
		}
		static token cosine() {
			return token("cos", 1, PRIORITY_FUNCTION, [](double a, double b) {return cos(a); });
		}
		static token tangent() {
			return token("tan", 1, PRIORITY_FUNCTION, [](double a, double b) {return tan(a); });
		}
		static token cotangent() {
			return token("cot", 1, PRIORITY_FUNCTION, [](double a, double b) {return 1 / tan(a); });
		}
		static token secant() {
			return token("sec", 1, PRIORITY_FUNCTION, [](double a, double b) {return 1 / cos(a); });
		}
		static token cosecant() {
			return token("csc", 1, PRIORITY_FUNCTION, [](double a, double b) {return 1 / sin(a); });
		}
		static token arcsine() {
			return token("arcsin", 1, PRIORITY_FUNCTION, [](double a, double b) {return asin(a); });
		}
		static token arccosine() {
			return token("arccos", 1, PRIORITY_FUNCTION, [](double a, double b) {return acos(a); });
		}
		static token arctangent() {
			return token("arctan", 1, PRIORITY_FUNCTION, [](double a, double b) {return atan(a); });
		}
		static token arccotangent() {
			return token("arccot", 1, PRIORITY_FUNCTION, [](double a, double b) {return atan(1 / a); });
		}
		static token arcsecant() {
			return token("arcsec", 1, PRIORITY_FUNCTION, [](double a, double b) {return acos(1 / a); });
		}
		static token arccosecant() {
			return token("arccsc", 1, PRIORITY_FUNCTION, [](double a, double b) {return asin(1 / a); });
		}
		static token common_logarithm() {
			return token("lg", 1, PRIORITY_FUNCTION, [](double a, double b) {return log10(a); });
		}
		static token natural_logarithm() {
			return token("ln", 1, PRIORITY_FUNCTION, [](double a, double b) {return log(a); });
		}
		static token square_root() {
			return token("sqrt", 1, PRIORITY_FUNCTION, [](double a, double b) {return sqrt(a); });
		}
		static token cubic_root() {
			return token("cbrt", 1, PRIORITY_FUNCTION, [](double a, double b) {return cbrt(a); });
		}
		static token degree() {
			return token("deg", 1, PRIORITY_FUNCTION, [](double a, double b) {return a / CONSTANT_PI * 180; });
		}
		static token radian() {
			return token("rad", 1, PRIORITY_FUNCTION, [](double a, double b) {return a / 180 * CONSTANT_PI; });
		}
		static token from_string(const std::string& str);
	private:
		static std::optional<double> try_parse_number(const std::string& str);
		static std::optional<token> try_parse_operator(const std::string& str);
	};
	class expression {
		std::vector<token> m_infix;
		std::vector<token> m_postfix;
	private:
		void calculate(std::stack<token>& operands, const token& op) const;
	public:
		expression(const std::string& infix_expression);
		std::string infix_expression() const;
		std::string postfix_expression() const;
		double evaluate_from_postfix() const;
		double evaluate_from_infix() const;
	};
}

#endif // CALCULATOR_HPP