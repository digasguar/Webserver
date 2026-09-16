#include "../includes/ConfigTokenizer.hpp"
#include <fstream>
#include <sstream>
#include <cctype>

// lee el archivo entero y lo separa en tokens: '{', '}', ';', 'xyz'.
// los comentarios empiezan con '#' y terminana con '/n'.
bool tokenizeConfigFile(const std::string &path, std::vector<std::string> &tokens)
{
    std::ifstream file(path.c_str());
    if (!file.is_open())
        return (false);
    std::stringstream buffer;
    buffer << file.rdbuf(); //vuelca todo el contenido en buffer sin usar gnl.
    std::string content = buffer.str();

    std::string word;
    for (size_t i = 0; i < content.size(); ++i)
    {
        char c = content[i];

        if (c == '#')
        {
            while (i < content.size() && content[i] != '\n')
                ++i;
            continue;
        }

        if (c == '{' || c == '}' || c == ';')
        {
            if (!word.empty())
            {
                tokens.push_back(word);
                word.clear();
            }
            tokens.push_back(std::string(1, c)); //convertir char a string de un caracter.
            continue;
        }

        if (std::isspace(static_cast<unsigned char>(c)))
        {
            if (!word.empty())
            {
                tokens.push_back(word);
                word.clear();
            }
            continue;
        }

        word += c;
    }
    if (!word.empty())
        tokens.push_back(word);
    return (true);
}
