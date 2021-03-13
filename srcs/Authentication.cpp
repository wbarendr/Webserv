#include "Authentication.hpp"

std::string         EncodePassword(std::string& password)
{
    std::string encoded;
    int index = 0;
    for (int i = 0; password[i]; ++i){
        if (password[i] > 100){
            encoded[index] = password[i] - 10; 
            index++;
            encoded[index] = password[i] - 22;
            index++;
        }
        else {
            encoded[index] = password[i] + 10; 
            index++;
            encoded[index] = password[i] + 22;
            index++;
        }
    }
    return encoded;
}

bool                Authenticated(Client& c)
{
    // if (!c.getRequest().m_location->auth_needed)
    //     return true;
    if (c.getRequest().m_headers[AUTHORIZATION] == ""){
        c.getRequest().m_error = 401;
        std::cout << "--------401---------" << std::endl;
        return false;
    }
    return true;
}

// void                Sethtpasswd