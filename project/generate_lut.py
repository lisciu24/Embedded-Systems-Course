import math
import matplotlib.pyplot as plt

LUT_SIZE = 50
DAC_MAX = 1023

def clamp(v, lo, hi):
    return max(lo, min(hi, v))

def generate_sin():
    array = []
    step = 360/LUT_SIZE

    for i in range(LUT_SIZE):

        # range [-1,1]
        s = math.sin(math.radians(i*step))

        # convert to [0,1]
        s = (s + 1.0) * 0.5

        value = int(round(s * DAC_MAX))

        array.append(clamp(value, 0, DAC_MAX))

    return array


def generate_triangle():
    array = []

    for i in range(LUT_SIZE):

        # p in range [0,1)
        p = i / LUT_SIZE

        # first quarter
        if p < 0.25:
            # 0.5 -> 1.0
            # y=a*x
            t = (p * (0.5 / 0.25)) + 0.5

        # second + third quarter
        elif p < 0.75:

            # 1.0 -> 0.0
            t =  -(p * (0.5 / 0.25)) + 1.5

        # fourth quarter
        else:

            # 0.0 -> 0.5
            t = (p * (0.5 / 0.25)) -1.5

        value = int(round(t * DAC_MAX))

        array.append(clamp(value, 0, DAC_MAX))

    return array


def generate_square():
    array = []

    for i in range(LUT_SIZE):
        if i < LUT_SIZE // 2:
            value = DAC_MAX
        else:
            value = 0

        array.append(value)

    return array


def print_c_array(name, data):
    print(f"const uint16_t {name}[{LUT_SIZE}] =")
    print("{")

    for i in range(0, len(data), 8): # 8 values in one line
        chunk = data[i:i+8]

        line = ", ".join(f"{v:4d}" for v in chunk)

        if i + 8 < len(data):
            print(f"    {line},")
        else:
            print(f"    {line}")

    print("};")
    print()

if __name__ == "__main__":
    sin = generate_sin()
    triangle = generate_triangle()
    square = generate_square()

    print("#include <stdint.h>")
    print()

    print_c_array("sin_lut", sin)
    print_c_array("triangle_lut", triangle)
    print_c_array("square_lut", square)

    plt.plot(sin, label="Sine")
    plt.plot(triangle, label="Triangle")
    plt.plot(square, label="Square")

    plt.title("DAC LUTs")

    plt.xlabel("Sample")
    plt.ylabel("DAC value")

    plt.legend()
    plt.grid(True)

    plt.show()
