import matplotlib.pyplot as plt
import re
import subprocess
import sys # 用于获取命令行参数
import time # 用于等待进程终止

# --- User Configuration Section ---
# 请在这里指定你的程序路径。
# 例如：'./my_bandwidth_test_program' (Linux/macOS) 或 'my_bandwidth_test_program.exe' (Windows)
# 如果你的程序在当前目录下，可以直接写程序名。
YOUR_PROGRAM_PATH = './mem' # <-- 请将这里替换为你的实际程序路径

# 你可以手动指定一个参数给你的程序。
# 例如：如果你的程序需要一个表示数组大小的参数，可以设置为 '8000'。
# 如果你的程序不需要任何参数，可以将其留空字符串 '' 或 None。
PROGRAM_PARAMETER = '12000' # <-- 根据你的程序需求修改或留空

# --- Script Logic Starts ---

def run_and_parse_program(program_path: str, parameter: str = None) -> tuple[list[int], list[float]]:
    """
    运行指定的外部程序，捕获其输出，并在达到指定循环数后终止程序，
    然后解析出循环编号和带宽数据。

    Args:
        program_path (str): 要执行的程序的可执行文件路径。
        parameter (str, optional): 传递给程序的命令行参数。默认为 None。

    Returns:
        tuple: 包含两个列表 (loop_numbers, bandwidths)。如果解析失败，返回 ([], [])。
    """
    command = [program_path]
    if parameter:
        command.append(parameter)

    print(f"Executing command: {' '.join(command)}")
    proc = None # 初始化进程对象
    program_output_lines = []
    max_loops_to_process = 10 # 目标：处理到 loop 10 就终止

    try:
        # 使用 Popen 启动程序，以便我们可以实时读取输出并控制进程
        proc = subprocess.Popen(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True, # 将输出视为文本
            encoding='utf-8',
            bufsize=1 # 行缓冲，确保能实时读取行
        )

        print("--- Capturing Program Output ---")
        # 逐行读取程序的标准输出
        while True:
            line = proc.stdout.readline()
            if not line: # 如果没有更多行可读 (程序可能已退出或管道关闭)
                break
            program_output_lines.append(line)
            print(f"Captured: {line.strip()}") # 打印捕获到的每一行，方便调试

            # 检查当前行是否包含循环编号，并判断是否达到终止条件
            match = re.search(r'loop (\d+) in .* \(bandwidth: ([\d.]+) MB/s\)', line)
            if match:
                current_loop_num = int(match.group(1))
                if current_loop_num >= max_loops_to_process:
                    print(f"\nReached loop {max_loops_to_process}, terminating program '{program_path}'...")
                    break # 达到目标循环数，跳出读取循环

        # 尝试终止进程
        if proc.poll() is None: # 检查进程是否仍在运行
            proc.terminate() # 发送 SIGTERM 信号，请求程序优雅地终止
            try:
                # 等待程序在一定时间内终止
                proc.wait(timeout=5)
                print("Program terminated gracefully.")
            except subprocess.TimeoutExpired:
                # 如果程序在超时时间内未能终止，则强制杀死
                print("Program did not terminate gracefully within 5 seconds, forcing kill...")
                proc.kill() # 发送 SIGKILL 信号，强制杀死程序
        else:
            print("Program already exited.")

        # 捕获程序终止后可能剩余的任何输出 (包括 stderr)
        remaining_stdout, remaining_stderr = proc.communicate()
        if remaining_stdout:
            program_output_lines.append(remaining_stdout)
            print(f"Captured remaining stdout:\n{remaining_stdout.strip()}")
        if remaining_stderr:
            print(f"Program produced stderr output:\n{remaining_stderr.strip()}")

    except FileNotFoundError:
        print(f"Error: Program not found at '{program_path}'. Please check the path and ensure it's executable.")
        return [], []
    except Exception as e:
        print(f"An unexpected error occurred while running the program: {e}")
        # 确保在发生任何异常时也尝试终止进程
        if proc and proc.poll() is None:
            print("Attempting to kill program due to error...")
            proc.kill()
        return [], []

    # 将所有收集到的输出行合并成一个字符串进行统一解析
    full_program_output = "".join(program_output_lines)
    print("\n--- Full Collected Program Output for Parsing ---")
    print(full_program_output)
    print("-------------------------------------------------\n")

    loop_numbers = []
    bandwidths = []

    # 解析收集到的完整输出
    for line in full_program_output.strip().split('\n'):
        match = re.search(r'loop (\d+) in .* \(bandwidth: ([\d.]+) MB/s\)', line)
        if match:
            try:
                loop_num = int(match.group(1))
                bandwidth = float(match.group(2))
                loop_numbers.append(loop_num)
                bandwidths.append(bandwidth)
            except ValueError as ve:
                print(f"Warning: Could not convert parsed values for line '{line}': {ve}")
        # 注意：这里不再为不匹配的行打印警告，以减少输出噪音

    return loop_numbers, bandwidths

def main():
    """
    Main function to orchestrate program execution, data parsing, and plotting.
    """
    global YOUR_PROGRAM_PATH, PROGRAM_PARAMETER # Access global variables for configuration

    # Get program path and parameter from command-line arguments if provided
    # Example: python plot_bandwidth.py ./my_program 1024
    if len(sys.argv) > 1:
        YOUR_PROGRAM_PATH = sys.argv[1]
        if len(sys.argv) > 2:
            PROGRAM_PARAMETER = sys.argv[2]
        else:
            PROGRAM_PARAMETER = None # If only program path is provided, no parameter
    else:
        print("No program path provided as command-line argument. Using default settings.")
        print(f"Default program path: {YOUR_PROGRAM_PATH}")
        print(f"Default parameter: {PROGRAM_PARAMETER}")
        print("You can run this script with: python your_script_name.py <program_path> [parameter]")

    # Run the program and get data
    loop_numbers, bandwidths = run_and_parse_program(YOUR_PROGRAM_PATH, PROGRAM_PARAMETER)

    # Check if data was parsed
    if not loop_numbers:
        print("Error: No valid data parsed from the program's output. Cannot generate graph.")
    else:
        # Generate filename and plot title based on the parameter value
        param_display_name = PROGRAM_PARAMETER if PROGRAM_PARAMETER is not None else "default"
        filename = f"bandwidth_array_size_{param_display_name}.png"
        plot_title = f'Loop Bandwidth Performance - Array Size: {param_display_name}'

        # Plot the graph
        plt.figure(figsize=(10, 6)) # Set graph size
        plt.plot(loop_numbers, bandwidths, marker='o', linestyle='-', color='b') # Plot line graph with circle markers

        # Add graph title and axis labels
        plt.title(plot_title, fontsize=16)
        plt.xlabel('Loop Number', fontsize=12)
        plt.ylabel('Bandwidth (MB/s)', fontsize=12)

        # Add grid lines for easier data viewing
        plt.grid(True, linestyle='--', alpha=0.7)

        # Display specific values for each data point
        for i, txt in enumerate(bandwidths):
            plt.annotate(f'{txt:.2f}', (loop_numbers[i], bandwidths[i]), textcoords="offset points", xytext=(0,10), ha='center')

        # Adjust layout to prevent labels from overlapping
        plt.tight_layout()

        # Save the graph to a file
        plt.savefig(filename)
        print(f"Graph saved successfully as '{filename}' in the current directory.")

        # Show the graph (optional, you can comment this line if you only want to save without displaying)
        plt.show()

        print("Graph generation and saving complete!")

# This block ensures that main() is called only when the script is executed directly
if __name__ == "__main__":
    main()

