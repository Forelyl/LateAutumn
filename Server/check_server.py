from enum import Enum
import socket
import struct
import time
import ctypes
import os


# ---------------------------------------------------------
def define_dll_libraries():
    global error_repair_lib

    os.add_dll_directory(r'D:/Programs/MinGW/bin')
    current_directory = os.path.dirname(os.path.abspath(__file__))
    path_to_dll = os.path.join(current_directory, "../.build/Network")
    os.add_dll_directory(path_to_dll)
    error_repair_lib = ctypes.WinDLL("liblate_autumn_error_repairing.dll")

    # extern "C" bool encode_package_c_wrapped (uint8_t* ptr_data, size_t size, uint8_t** result, size_t* result_size);
    error_repair_lib.encode_package_c_wrapped.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)), ctypes.POINTER(ctypes.c_size_t)]
    error_repair_lib.encode_package_c_wrapped.restype  = ctypes.c_bool

    # extern "C" void free_c_wrapped (void* ptr);
    error_repair_lib.free_c_wrapped.argtypes = [ctypes.c_void_p]
    error_repair_lib.free_c_wrapped.restype  = None

    # extern "C" bool decode_package_c_wrapped (uint8_t* ptr_data, size_t size, uint8_t** result, size_t* result_size);
    error_repair_lib.decode_package_c_wrapped.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(ctypes.POINTER(ctypes.c_uint8)), ctypes.POINTER(ctypes.c_size_t)]
    error_repair_lib.decode_package_c_wrapped.restype  = ctypes.c_bool


define_dll_libraries()


def define_dll_libraries():
    pass


# ---------------------------------------------------------

# extern "C" bool encode_package_c_wrapped (uint8_t* ptr_data, size_t size, uint8_t** result, size_t* result_size);
def encode_data(data: bytes) -> bytes | None:
    data = (ctypes.c_uint8 * len(data))(*data)
    size = ctypes.c_size_t(len(data))

    result      = ctypes.POINTER(ctypes.c_uint8)()
    result_size = ctypes.c_size_t()

    is_ok: ctypes.c_bool = error_repair_lib.encode_package_c_wrapped(data, size, ctypes.byref(result), ctypes.byref(result_size))
    if not is_ok:
        return None

    data = bytearray(result[:result_size.value])
    error_repair_lib.free_c_wrapped(ctypes.cast(result, ctypes.c_void_p))
    return bytes(data)


# extern "C" bool decode_package_c_wrapped (uint8_t* ptr_data, size_t size, uint8_t** result, size_t* result_size);
def decode_data(data: bytes) -> bytes | None:
    data = (ctypes.c_uint8 * len(data))(*data)
    size = ctypes.c_size_t(len(data))

    result      = ctypes.POINTER(ctypes.c_uint8)()
    result_size = ctypes.c_size_t()

    is_ok: ctypes.c_bool = error_repair_lib.decode_package_c_wrapped(data, size, ctypes.byref(result), ctypes.byref(result_size))
    if not is_ok:
        return None

    data = bytearray(result[:result_size.value])
    error_repair_lib.free_c_wrapped(ctypes.cast(result, ctypes.c_void_p))

    return bytes(data)

# ------------------------------------------


class ANSWER(Enum):
    ERROR_VALUE_INCORRECT   = "ERROR_VALUE_INCORRECT",
    BAD_FORMED              = "BAD_FORMED",
    REGISTERED              = "REGISTERED",
    ACKNOWLEDGE             = "ACKNOWLEDGE",
    OTHER                   = "OTHER",
    FINISH                  = "FINISH",
    BREAK_SESSION           = "BREAK_SESSION"


answer_type: dict[int, ANSWER] = {
    -2: ANSWER.ERROR_VALUE_INCORRECT,
    -1: ANSWER.BAD_FORMED,
    0: ANSWER.REGISTERED,
    1: ANSWER.ACKNOWLEDGE,
    2: ANSWER.OTHER,
    3: ANSWER.FINISH,
    4: ANSWER.BREAK_SESSION,
}


def parse_info(data) -> None:
    print("Answer from server", data)
    offset_of_binary_data = 0

    # type
    temp_data = struct.unpack_from('b', data, offset_of_binary_data)[0]
    offset_of_binary_data += 1

    if temp_data not in answer_type:
        print("Type: unknown")
        return

    message_type = answer_type[temp_data]
    print("Response type: ", message_type.value)

    # no payload
    if temp_data not in [ANSWER.ACKNOWLEDGE, ANSWER.FINISH, ANSWER.OTHER]: return

    # payload
    if message_type is ANSWER.ACKNOWLEDGE: # id
        temp_data = struct.unpack_from('!Q', data, offset_of_binary_data)[0]
        offset_of_binary_data += 8
        print("Acknowledged package ID: ", temp_data)

    elif message_type is ANSWER.OTHER: # (flag data)+
        pass
    elif message_type is ANSWER.FINISH: # true/false
        pass


# Define the message type codes
MESSAGE_TYPE_SETUP         = 0
MESSAGE_TYPE_MESSAGE       = 1
MESSAGE_TYPE_GET_OTHER     = 2
MESSAGE_TYPE_CHECK_FINISH  = 3
MESSAGE_TYPE_BREAK_SESSION = 4


class Client:

    def __init__(self):
        self.socket: socket.socket | None = None
        self.server_address = ('127.0.0.1', 20123)
        self.registered: bool = False

    def __del__(self):
        if self.socket is not None:
            self.socket.close()

    def make_request_read_answer(self, message: bytes):
        # Send the message
        self.socket.sendto(encode_data(message), self.server_address)
        print("Message sent\n")

        # Get answer
        answer = self.socket.recvfrom(1024)
        print("Answer from server", answer)
        parse_info(decode_data(answer[0]))

    def create_socket(self):
        if self.socket is not None:
            self.socket.close()
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.bind(('127.0.0.1', 0))
        self.registered = False

    def message(self):
        if self.socket is None: return

        # Define the fields
        message_type = MESSAGE_TYPE_MESSAGE  # Set this to the type you want to test
        message_id = 1                       # Example message ID
        x  = 12.34                           # Example x-coordinate
        y  = 56.78                           # Example y-coordinate
        dx = 1                               # Example x-speed
        dy = 10.5                            # Example y-speed
        timestamp = int(time.time())         # Example timestamp (current time)

        # Pack the data
        # Format: B for 1 byte (message type), Q for 8 bytes (long), d for 8 bytes (double)
        # Total size: 1 + 8 + 8 + 8 + 8 + 8 + 8 = 49 bytes
        message = struct.pack('!B Q d d d d q', message_type, message_id, x, y, dx, dy, timestamp)
        print(
            "Message (len: " + str(len(message)) + "): "
            + "\n\tmesssage_type: " + str(message_type)
            + "\n\tmessage_id: " + str(message_id)
            + "\n\tx: " + str(x)
            + "\n\ty: " + str(y)
            + "\n\tdx: " + str(dx)
            + "\n\tdy: " + str(dy)
            + "\n\ttimestamp: " + str(timestamp)
            + "\n"
        )
        self.make_request_read_answer(message)

    def register(self):
        if self.socket is None:
            print("No socket")
            return

        if self.registered:
            print("Client already registered")
            return

        # Define the fields
        message_type = MESSAGE_TYPE_SETUP  # Set this to the type you want to test

        # Pack the data
        # Total size: 1 = 1 byte
        message = struct.pack('!B', message_type)
        print(
            "Message (len: " + str(len(message)) + "): "
            + "\n\tmesssage_type: " + str(message_type) + "\n"
        )
        self.make_request_read_answer(message)
        self.registered = True

    def delete(self):
        if self.socket is None:
            print("No socket")
            return

        if not self.registered:
            print("Client not registered")
            return

        message_type = MESSAGE_TYPE_BREAK_SESSION  # Set this to the type you want to test

        # Pack the data
        # Total size: 1 = 1 byte
        message = struct.pack('!B', message_type)
        print(
            "Message (len: " + str(len(message)) + "): "
            + "\n\tmesssage_type: " + str(message_type) + '\n'
        )
        self.make_request_read_answer(message)# Example timestamp (current time)
        self.registered = False

    def ask_about_other(self):
        if self.socket is None:
            print("No socket")
            return
        if not self.registered:
            print("Client not registered")
            return

        message_type = MESSAGE_TYPE_GET_OTHER
        message_id   = 2 # Example message ID

        # Pack the data
        # Total size: 1 = 1 byte
        message = struct.pack('!B Q', message_type, message_id)
        print(
            "Message (len: " + str(len(message)) + "): "
            + "\n\tmesssage_type: " + str(message_type) + '\n'
        )
        self.make_request_read_answer(message)

    def get_socket_info(self) -> str:
        result = ""
        if self.socket is None:
            return "No information"
        else:
            ip, port = self.socket.getsockname()
            result += str(ip) + ":" + str(port)

        if self.registered:
            result += " (registered)"
        else:
            result += " (not registered)"

        return result


# ============================================

def unix_clear():
    print('\n' * 20)


def obtain_int(pre_text: str) -> int:
    while True:
        unix_clear()
        print(pre_text)
        try:
            a = input("Input (only integer): ")
        except:
            pass
        if a.isdecimal():
            return int(a)


class terminal_options(Enum):
    CREATE_SOCKET       = 1
    REGISTER            = 2
    SEND_NICE_DATA      = 3
    SEND_BAD_DATA       = 4
    ASK_ABOUT_FINISH    = 5
    ASK_ABOUT_OTHER     = 6
    DESTROY_CONNECTION  = 7
    EXIT = 8


def main_func() -> None:
    client: Client = Client()
    main_text = (
        "================ [Fast blade client - late autumn] ================\n" +
        "{}\n" +
        "1. Create socket\n" +
        "2. Register\n" +
        "3. Send nice data (small move)\n" +
        "4. Send incorrect data\n" +
        "5. Ask about finish\n" +
        "6. Ask about other\n" +
        "7. Destroy connection\n" +
        "8. Exit\n" +
        "================ [Fast blade client - late autumn] ================"
    )

    while True:
        main_input = obtain_int(main_text.format(client.get_socket_info()))
        if main_input > 8 or main_input < 1:
            continue

        match terminal_options(main_input):
            case terminal_options.CREATE_SOCKET:
                client.create_socket()
            case terminal_options.REGISTER:
                client.register()
            case terminal_options.SEND_NICE_DATA:
                client.message()
            case terminal_options.SEND_BAD_DATA:
                pass
            case terminal_options.ASK_ABOUT_FINISH:
                pass
            case terminal_options.ASK_ABOUT_OTHER:
                client.ask_about_other()
            case terminal_options.DESTROY_CONNECTION:
                client.delete()
            case terminal_options.EXIT:
                if client.registered: client.delete()
                return

        input("Press enter to continue...")


if __name__ == "__main__":
    main_func()