/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_UTILS_PRETTY_FUNCTION_H
#define FLATCFG_UTILS_PRETTY_FUNCTION_H

#include <algorithm>
#include <cstddef>
#include <string_view>

/// @brief Custom __PRETTY_FUNCTION__ without parameters, return type, or namespaces.
#define FLATCFG_PRETTY_FUNCTION flatcfg::utils::PrettyFunction::Create(__PRETTY_FUNCTION__)

namespace flatcfg
{
namespace utils
{

/// @brief Functions to implement custom __PRETTY_FUNCTION__ without parameters, return type, or namespaces.
/// @note  This is intentionally header only since there's no point in mocking pure functions.
class PrettyFunction
{
public:
    /// @brief Call all functions in correct order, and return our own version of __PRETTY_FUNCTION__.
    static inline constexpr std::string_view
    Create(std::string_view sv) noexcept
    {
        return _trim(_removeNamespaces(_removeReturnType(_removeFunctionParams(_removeTemplateTypesList(sv)))));
    }

    /// @brief Removes preceding and trailing space characters.
    static inline constexpr std::string_view
    // coverity[autosar_cpp14_a15_5_2_violation] The design of the program does not raise the std::terminate exception.
    // coverity[autosar_cpp14_a15_5_3_violation] The design of the program does not raise the std::terminate exception.
    // coverity[exn_spec_violation] No try-catch is required because the design of the program does not raise exceptions.
    _trim(std::string_view sv) noexcept
    {
        // find preceding spaces
        std::size_t posPre { 0U };
        while ((posPre < sv.size()) && (sv[posPre] == ' '))
        {
            ++posPre;
        }

        // find trailing spaces
        std::size_t posPost { sv.size() - 1U };
        while ((posPost >= 1U) && (sv[posPost] == ' '))
        {
            --posPost;
        }

        // remove spaces
        // coverity[autosar_cpp14_a15_4_2_violation] The design of the program does not raise the std::out_of_range exception.
        return sv.substr(0U, posPost + 1U).substr(std::min(posPre, sv.size()));
    }

    /// @brief   Removes template types list, if it exists, from a string obtained from __PRETTY_FUNCTION__.
    ///          example:'T<U, X>::T() [with U = int; X = std::vector]' -> 'T<U, X>::T()'.
    /// @pre     Input must be obtained directly from __PRETTY_FUNCTION__.
    static inline constexpr std::string_view
    // coverity[autosar_cpp14_a15_5_2_violation] The design of the program does not raise the std::terminate exception.
    // coverity[autosar_cpp14_a15_5_3_violation] The design of the program does not raise the std::terminate exception.
    // coverity[exn_spec_violation] No try-catch is required because the design of the program does not raise exceptions.
    _removeTemplateTypesList(std::string_view sv) noexcept
    {
        // remove template types: [with U = int; X = std::vector]
        std::size_t pos { 0U };
        for (; pos < sv.size(); ++pos)
        {
            // find opening of []
            if (sv[pos] == '[')
            {
                // check that we're not on the first character (shouldn't be possible if pre-conditions are followed)
                if (pos == 0U)
                {
                    // return directly with no modifications
                    return sv;
                }

                // check that we've found template type list and not operator[]
                // template type list will have a space preceding it
                // e.g. T<U, X>::T() [with U = int; X = std::vector]
                //      -------------^------------------------------
                else if (sv[pos - 1U] == ' ')
                {
                    // put pos on space before []
                    --pos;
                    break;
                }

                // found operator[]; skip
                // e.g. T<U, X>::operator[]()
                //      -----------------^---
                else
                {
                    continue;
                }
            }
        }

        // position is currently on opening bracket
        // e.g. T<U, X>::T() [with U = int; X = std::vector]
        //      -------------^------------------------------

        // return modified view
        // coverity[autosar_cpp14_a15_4_2_violation] The design of the program does not raise the std::out_of_range exception.
        return _trim(sv.substr(0U, pos));
    }

    /// @brief   Removes function params, if they exist, from a string obtained from _removeTemplateTypesList(...).
    ///          example: 'operator[](int (*)())' -> 'operator[]'.
    /// @pre     Input must be obtained directly from _removeTemplateTypesList(...).
    static inline constexpr std::string_view
    // coverity[autosar_cpp14_a15_5_2_violation] The design of the program does not raise the std::terminate exception.
    // coverity[autosar_cpp14_a15_5_3_violation] The design of the program does not raise the std::terminate exception.
    // coverity[exn_spec_violation] No try-catch is required because the design of the program does not raise exceptions.
    _removeFunctionParams(std::string_view sv) noexcept
    {
        // remove all characters from end until first closing brace
        // potentially the function is const, volatile, or reference qualified
        // e.g. int T::foo() const
        //      -----------------^
        while (!sv.empty() && sv.back() != ')')
        {
            sv = sv.substr(0U, sv.size() - 1U);
        }

        // check that input is not empty
        if (sv.empty())
        {
            return sv;
        }

        // position is currently on outer closing brace
        // e.g. (int (*)())
        //      ----------^
        std::size_t pos { sv.size() - 1U };

        // keep track of nested ()
        std::ptrdiff_t stackSize { 1 };

        // remove function params: (int (*)())
        // do '>= 1U' to avoid unsigned overflow issues
        for (; (stackSize > 0) && (pos >= 1U); --pos)
        {
            // found new nested closing brace
            // e.g. (int (*)())
            //      ---------^-
            if (sv[pos - 1U] == ')')
            {
                ++stackSize;
            }

            // found new nested opening brace
            // e.g. (int (*)())
            //      --------^--
            else if (sv[pos - 1U] == '(')
            {
                --stackSize;
            }

            // found normal character; skip
            else
            {
                continue;
            }
        }

        // position is currently on outer opening brace
        // e.g. (int (*)())
        //      ^----------

        // return modified view
        // coverity[autosar_cpp14_a15_4_2_violation] The design of the program does not raise the std::out_of_range exception.
        return _trim(sv.substr(0U, pos));
    }

    /// @brief   Removes function return types from a string obtained from _removeFunctionParams(...).
    ///          example: 'void ns::detail::T<U, X>::operator[]' -> 'ns::detail::T<U, X>::operator[]'.
    /// @pre     Input must be obtained directly from _removeFunctionParams(...).
    static inline constexpr std::string_view
    // RULECHECKER_comment(4, 1, check_max_control_nesting_depth, "Nested conditionals formatted clearly, make understanding intention easier for reviewers.", true);
    // coverity[autosar_cpp14_a15_5_2_violation] The design of the program does not raise the std::terminate exception.
    // coverity[autosar_cpp14_a15_5_3_violation] The design of the program does not raise the std::terminate exception.
    // coverity[exn_spec_violation] No try-catch is required because the design of the program does not raise exceptions.
    _removeReturnType(std::string_view sv) noexcept
    {
        // check that input end is not on a space
        if (sv.empty() || sv.back() == ' ')
        {
            return sv;
        }

        // position is currently on end of function name
        // e.g. void ns::outer<T, U>::inner<V, W>::fn  OR  int main
        //      ------------------------------------^      -------^
        std::size_t pos { sv.size() - 1U };

        // keep track of nested <>
        std::ptrdiff_t stackSize { 0 };

        // go to start of function name, including enclosing classes and namespaces
        // do this by finding first space where stackSize == 0
        // do '>= 1U' to avoid unsigned overflow issues
        for (; pos >= 1U; --pos)
        {
            // found new nested closing angle bracket
            // e.g. void ns::outer<T, U>::inner<V, W>::fn
            //      --------------------------------^----
            if (sv[pos - 1U] == '>')
            {
                // coverity[autosar_cpp14_a4_7_1_violation] it is used only for constexpr functions and for processing debug log results
                ++stackSize;
            }

            // found new nested opening angle bracket
            // e.g. void ns::outer<T, U>::inner<V, W>::fn
            //      ---------------------------^---------
            else if (sv[pos - 1U] == '<')
            {
                // coverity[autosar_cpp14_a4_7_1_violation] it is used only for constexpr functions and for processing debug log results
                --stackSize;
            }

            // found a space
            else if (sv[pos - 1U] == ' ')
            {
                // stack size is 0
                // e.g. void ns::outer<T, U>::inner<V, W>::fn
                //      ----^--------------------------------
                if (stackSize == 0)
                {
                    break;
                }

                // stack size is not 0
                // e.g. void ns::outer<T, U>::inner<V, W>::fn
                //      ------------------------------^------
                else
                {
                    continue;
                }
            }

            // found a normal character; skip
            else
            {
                continue;
            }
        }

        // position is currently at start of function name, including enclosing classes and namespaces
        // e.g. void ns::outer<T, U>::inner<V, W>::fn  OR  int main
        //      -----^-------------------------------      ----^---

        // return modified view
        // coverity[autosar_cpp14_a15_4_2_violation] The design of the program does not raise the std::out_of_range exception.
        return _trim(sv.substr(pos));
    }

    /// @brief   Removes function namespaces from a string obtained from _removeReturnType(...).
    ///         example: 'ns::detail::T::operator[]' -> 'T::operator[]'.
    /// @pre     Input must be obtained directly from _removeReturnType(...).
    /// @pre     All namespaces must be 'snake_case' and all class names must be 'PascalCase'.
    static inline constexpr std::string_view
    // coverity[autosar_cpp14_a15_5_2_violation] The design of the program does not raise the std::terminate exception.
    // coverity[autosar_cpp14_a15_5_3_violation] The design of the program does not raise the std::terminate exception.
    // coverity[exn_spec_violation] No try-catch is required because the design of the program does not raise exceptions.
    _removeNamespaces(std::string_view sv) noexcept
    {
        // check that input end is not a space and start is not a colon
        if (sv.empty() || sv.back() == ' ' || sv.front() == ':')
        {
            return sv;
        }

        // position is currently at start of symbol
        // e.g. ns::_detail::Class<T>::fn
        //      ^------------------------
        std::size_t pos { 0U };

        // detect namespaces by checking if start of input matches '[a-z0-9_]+::'
        // everytime we match that regex, we remove that prefix
        while (true)
        {
            std::size_t tempPos { pos };

            // warning: technically C++14 does not guarantee that a-z numerical values are sequential
            while ((tempPos < sv.size()) && ((sv[tempPos] >= 'a' && sv[tempPos] <= 'z') ||
                                             (sv[tempPos] >= '0' && sv[tempPos] <= '9') || (sv[tempPos] == '_')))
            {
                ++tempPos;
            }

            // check next characters are ::
            // e.g. ns::_detail::Class<T>::fn
            //      --^----------------------
            if ((tempPos <= sv.size() - 2U) && (sv[tempPos] == ':') && (sv[tempPos + 1U] == ':'))
            {
                tempPos += 2U;
                pos = tempPos;
            }
            // no namespaces remaining
            else
            {
                break;
            }
        }

        // position is currently after :: after last enclosing namespace
        // e.g. ns::_detail::Class<T>::fn
        //      -------------^-----------

        // return modified view
        // coverity[autosar_cpp14_a15_4_2_violation] The design of the program does not raise the std::out_of_range exception.
        return _trim(sv.substr(pos));
    }
};

}  // namespace utils
}  // namespace flatcfg

#endif  // FLATCFG_UTILS_PRETTY_FUNCTION_H
