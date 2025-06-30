CC = gcc
CFLAGS = -g -Wall

# 1. 定义所有可执行文件
TARGETS = memory-user

.PHONY: all clean

# 2. 默认的构建目标，依赖于所有的 TARGETS
all: $(TARGETS)

# 3. 使用模式规则：将 %.c 文件编译为 % 可执行文件
# 这条规则会应用于 TARGETS 中的每一个目标 (prog1, prog2)
%: %.c
	$(CC) $(CFLAGS) $< -o $@

# 4. clean 规则清理所有定义的 TARGETS
clean:
	rm -f $(TARGETS)