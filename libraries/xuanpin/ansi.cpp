#include <string>

#include <xuanpin/ansi.hpp>

namespace taiyi { namespace xuanpin {

    void string_replace_all(std::string& str, const std::string& from, const std::string& to) {
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    }
    //=============================================================================
    // 清理ANSI色彩标记
    void remove_ansi(std::string& arg)
    {
        string_replace_all(arg, BLK, "");
        string_replace_all(arg, RED, "");
        string_replace_all(arg, GRN, "");
        string_replace_all(arg, YEL, "");
        string_replace_all(arg, BLU, "");
        string_replace_all(arg, MAG, "");
        string_replace_all(arg, CYN, "");
        string_replace_all(arg, WHT, "");
        string_replace_all(arg, HIR, "");
        string_replace_all(arg, HIG, "");
        string_replace_all(arg, HIY, "");
        string_replace_all(arg, HIB, "");
        string_replace_all(arg, HIM, "");
        string_replace_all(arg, HIC, "");
        string_replace_all(arg, HIW, "");
        string_replace_all(arg, NOR, "");
        string_replace_all(arg, BOLD, "");
        string_replace_all(arg, BLINK, "");
        string_replace_all(arg, BBLK, "");
        string_replace_all(arg, BRED, "");
        string_replace_all(arg, BGRN, "");
        string_replace_all(arg, BYEL, "");
        string_replace_all(arg, BBLU, "");
        string_replace_all(arg, BMAG, "");
        string_replace_all(arg, BCYN, "");
        string_replace_all(arg, HBRED, "");
        string_replace_all(arg, HBGRN, "");
        string_replace_all(arg, HBYEL, "");
        string_replace_all(arg, HBBLU, "");
        string_replace_all(arg, HBMAG, "");
        string_replace_all(arg, HBCYN, "");
        string_replace_all(arg, HBWHT, "");
        string_replace_all(arg, HIDDEN, "");
    }
    //=============================================================================
    // 转义&ANSI码&为ANSI色彩输出
    void ansi(std::string& content)
    {
        if (content.empty())
            return;
        
        // Foreground color
        string_replace_all(content, "&BLK&", BLK);
        string_replace_all(content, "&RED&", RED);
        string_replace_all(content, "&GRN&", GRN);
        string_replace_all(content, "&YEL&", YEL);
        string_replace_all(content, "&BLU&", BLU);
        string_replace_all(content, "&MAG&", MAG);
        string_replace_all(content, "&CYN&", CYN);
        string_replace_all(content, "&HIK&", HIK);
        string_replace_all(content, "&WHT&", WHT);
        string_replace_all(content, "&HIR&", HIR);
        string_replace_all(content, "&HIG&", HIG);
        string_replace_all(content, "&HIY&", HIY);
        string_replace_all(content, "&HIB&", HIB);
        string_replace_all(content, "&HIM&", HIM);
        string_replace_all(content, "&HIC&", HIC);
        string_replace_all(content, "&HIW&", HIW);
        string_replace_all(content, "&NOR&", NOR);
        
        // Background color
        string_replace_all(content, "&BBLK&", BBLK);
        string_replace_all(content, "&BRED&", BRED);
        string_replace_all(content, "&BGRN&", BGRN);
        string_replace_all(content, "&BYEL&", BYEL);
        string_replace_all(content, "&BBLU&", BBLU);
        string_replace_all(content, "&BMAG&", BMAG);
        string_replace_all(content, "&BCYN&", BCYN);
        string_replace_all(content, "&HBRED&", HBRED);
        string_replace_all(content, "&HBGRN&", HBGRN);
        string_replace_all(content, "&HBYEL&", HBYEL);
        string_replace_all(content, "&HBBLU&", HBBLU);
        string_replace_all(content, "&HBMAG&", HBMAG);
        string_replace_all(content, "&HBCYN&", HBCYN);
        
        string_replace_all(content, "&U&", U);
        string_replace_all(content, "&BLINK&", BLINK);
        string_replace_all(content, "&REV&", REV);
        string_replace_all(content, "&HIREV&", HIREV);
        string_replace_all(content, "&BOLD&", BOLD);
        string_replace_all(content, "&HIDDEN&", HIDDEN);
    }
    //=============================================================================
    // calculate the color char in a string
    int color_len(const std::string& str)
    {
        int extra = 0;
        for (size_t i = 0; i < str.size(); i++)
        {
            if (str[i] == ESC[0])
            {
                while ((extra++, str[i] != 'm') && i < str.size())
                    i++;
            }
        }
        return extra;
    }
    //=============================================================================
    void color_to_html(std::string& msg)
    {
        if (msg.empty())
            return;
        
        string_replace_all(msg, BLK, "<span style=\"color: #000000\">");
        string_replace_all(msg, RED, "<span style=\"color: #990000\">");
        string_replace_all(msg, GRN, "<span style=\"color: #009900\">");
        string_replace_all(msg, YEL, "<span style=\"color: #999900\">");
        string_replace_all(msg, BLU, "<span style=\"color: #000099\">");
        string_replace_all(msg, MAG, "<span style=\"color: #990099\">");
        string_replace_all(msg, CYN, "<span style=\"color: #669999\">");
        string_replace_all(msg, WHT, "<span style=\"color: #EEEEEE\">");
        
        string_replace_all(msg, HIK, "<span style=\"color: #BBBBBB\">");
        string_replace_all(msg, HIR, "<span style=\"color: #FF0000\">");
        string_replace_all(msg, HIG, "<span style=\"color: #00FF00\">");
        string_replace_all(msg, HIY, "<span style=\"color: #FFFF00\">");
        string_replace_all(msg, HIB, "<span style=\"color: #0000FF\">");
        string_replace_all(msg, HIM, "<span style=\"color: #FF00FF\">");
        string_replace_all(msg, HIC, "<span style=\"color: #00FFFF\">");
        string_replace_all(msg, HIW, "<span style=\"color: #FFFFFF\">");
        
        string_replace_all(msg, BBLK, "<span style=\"background-color: #FFFF00\">");
        string_replace_all(msg, BRED, "<span style=\"background-color: #990000\">");
        string_replace_all(msg, BGRN, "<span style=\"background-color: #009900\">");
        string_replace_all(msg, BYEL, "<span style=\"background-color: #999900\">");
        string_replace_all(msg, BBLU, "<span style=\"background-color: #000099\">");
        string_replace_all(msg, BMAG, "<span style=\"background-color: #990099\">");
        string_replace_all(msg, BCYN, "<span style=\"background-color: #669999\">");
        string_replace_all(msg, BWHT, "<span style=\"background-color: #EEEEEE\">");
        
        string_replace_all(msg, NOR, "</span>");
        string_replace_all(msg, U, "<span>");
        string_replace_all(msg, BLINK, "<span>");
        string_replace_all(msg, REV, "<span>");
    }

} }// taiyi::xuanpin
