#include "xiaozhi_data.h"

// [CLAUDE_OPT] 定义全局变量实体，头文件中 extern 声明引用的就是这个
xiaozhi_data_t xiaozhi_data;

// 以下是常用的 emoji 列表。
/*
1. 😶 - neutral
2. 🙂 - happy
3. 😆 - laughing
4. 😂 - funny
5. 😔 - sad
6. 😠 - angry
7. 😭 - crying
8. 😍 - loving
9. 😳 - embarrassed
10. 😲 - surprised
11. 😱 - shocked
12. 🤔 - thinking
13. 😉 - winking
14. 😎 - cool
15. 😌 - relaxed
16. 🤤 - delicious
17. 😘 - kissy
18. 😏 - confident
19. 😴 - sleepy
20. 😜 - silly
21. 🙄 - confused
*/
EMOJI xiaozhi_emoji[24] =
    {
        {.emoji = "😶", .text = "neutral"},
        {.emoji = "🙂", .text = "happy"},
        {.emoji = "😆", .text = "laughing"},
        {.emoji = "😂", .text = "funny"},
        {.emoji = "😔", .text = "sad"},
        {.emoji = "😠", .text = "angry"},
        {.emoji = "😭", .text = "crying"},
        {.emoji = "😍", .text = "loving"},
        {.emoji = "😳", .text = "embarrassed"},
        {.emoji = "😲", .text = "surprised"},
        {.emoji = "😱", .text = "shocked"},
        {.emoji = "🤔", .text = "thinking"},
        {.emoji = "😉", .text = "winking"},
        {.emoji = "😎", .text = "cool"},
        {.emoji = "😌", .text = "relaxed"},
        {.emoji = "🤤", .text = "delicious"},
        {.emoji = "😘", .text = "kissy"},
        {.emoji = "😏", .text = "confident"},
        {.emoji = "😴", .text = "sleepy"},
        {.emoji = "😜", .text = "silly"},
        {.emoji = "🙄", .text = "confused"}};
