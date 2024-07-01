from collections import Counter
from typing import Dict, Generator, List, Union

import regex
from _bpe import build_trie, encode_inference, encode_train, manual_free_trie, train

__version__ = "1.0"

GPT2_REGEX_PATTERN = (
    r"""'(?:[sdmt]|ll|ve|re)| ?\p{L}+| ?\p{N}+| ?[^\s\p{L}\p{N}]+|\s+(?!\S)|\s+"""
)


class Tokenizer:
    """
    bytephase: A byte pair encoding (BPE) tokenizer with customizable regex pattern and C extension acceleration.

    This tokenizer implements a BPE algorithm for text tokenization, with support
    for training on input data, encoding and decoding text, and saving/loading
    the trained model. It utilizes C extensions for improved performance in critical operations.

    Attributes:
        pattern (str): Regex pattern used for tokenization.
        compiled_pattern (regex.Pattern): Compiled regex pattern.
        file_read_buffer (int): Size of the buffer used when reading files during training.
        decode_dict (dict): Mapping of token IDs to byte sequences.
        _trie: Internal trie structure for efficient encoding (C extension).

    Note:
        The tokenizer uses a trie data structure implemented in C for fast encoding.
        The trie is automatically freed when the Tokenizer instance is deleted.
    """

    __slots__ = (
        "pattern",
        "compiled_pattern",
        "file_read_buffer",
        "decode_dict",
        "_trie",
    )

    def __init__(
        self, pattern: Union[str, None] = None, file_read_buffer: int = 2097152
    ) -> None:
        """
        Initialize the Tokenizer with an optional regex pattern and buffer size.

        Args:
            pattern (str, optional): Custom regex pattern for tokenization.
                If None, uses the default GPT-2 pattern. Defaults to None.
            file_read_buffer (int, optional): Size of the buffer (in bytes) used when
                reading files for tokenization. Defaults to 2,097,152 (2MB).
        """

        self.pattern = GPT2_REGEX_PATTERN if pattern is None else pattern
        self.compiled_pattern = regex.compile(self.pattern)
        self.file_read_buffer = file_read_buffer
        self.decode_dict: Dict[int, bytes] = {}
        self._trie = None

    def __del__(self):
        if self._trie is not None:
            manual_free_trie(self._trie)
            self._trie = None

    def _read_file_in_chunks(self, file_path) -> Generator:
        """Generator function to read a file in chunks."""

        with open(file_path, "rb") as f:
            while True:
                chunk = f.read(self.file_read_buffer)
                if not chunk:
                    break
                yield chunk

    def _process_chunks(self, file_path) -> Generator:
        """Process chunks of the file using the provided regex pattern."""

        buffer = ""
        for chunk in self._read_file_in_chunks(file_path):
            chunk_str = chunk.decode("utf-8", errors="ignore")
            combined_data = buffer + chunk_str
            matches = self.compiled_pattern.findall(combined_data)

            if len(matches) > 0:
                yield matches[:-1]

            if matches:
                buffer = matches[-1]
            else:
                buffer = ""

        if buffer:
            yield buffer

    def train(self, file_path: str, vocab_size: int) -> None:
        """
        Train the tokenizer on the given file using the BPE algorithm.

        This method processes the input file in chunks, builds a frequency dictionary
        of tokens, and then uses the BPE algorithm to create a vocabulary of the
        specified size. It updates the tokenizer's decode dictionary and builds
        a trie structure for efficient encoding.

        Args:
            file_path (str): The path to the file containing the training data.
            vocab_size (int): The desired size of the final vocabulary.

        Raises:
            ValueError: If file_path is not a string or vocab_size is not a positive integer.

        Note:
            The resulting vocabulary includes 256 byte tokens plus additional merged tokens.
        """

        if not isinstance(file_path, str):
            raise ValueError("Input data must be a file path as a string")
        if not isinstance(vocab_size, int) or vocab_size <= 0:
            raise ValueError("vocab_size must be a positive integer")

        text_stats = Counter()
        for matches in self._process_chunks(file_path):
            text_stats.update(matches)
        text_stats = dict(text_stats)

        num_merges = vocab_size - 257
        merges = train(text_stats, len(text_stats), num_merges)

        self.decode_dict = {idx: bytes([idx]) for idx in range(256)}
        self.decode_dict[256] = bytes("<|endoftext|>".encode("utf-8"))

        idx = 257
        for merge in merges:
            byte_array = bytes(merge)
            self.decode_dict[idx] = byte_array
            idx += 1

        self._trie = build_trie(self.decode_dict)

    def encode(self, input_text: str, train_mode: bool = True) -> List[int]:
        """
        Encode the input text into a list of token IDs using a C-based trie structure.

        This method first tokenizes the input text using the compiled regex pattern,
        then encodes these tokens into token IDs using the C extension.

        Args:
            input_text (str): The input text to encode.
            train_mode (bool, optional): Flag to indicate if the encoding is in training mode. Defaults to True.

        Returns:
            List[int]: A list of token IDs representing the encoded text.

        Raises:
            ValueError: If input_text is not a string.

        Note:
            Encoding when train_mode is True will use less memory, but is slower by about 25% on average.
            When train_mode is False, encoding will be faster but use more memory, which is more appropriate
            at inference time.
        """
        if not isinstance(input_text, str):
            raise ValueError("Input text must be a string")

        if train_mode:
            chunk_iterator = self.compiled_pattern.finditer(input_text)
            return encode_train(chunk_iterator, self._trie)

        else:
            text_chunks = self.compiled_pattern.findall(input_text)
            return encode_inference(text_chunks, self._trie)

    def decode(self, input_tokens: List[int]) -> str:
        """
        Decode a list of token IDs back into text.

        Args:
            input_tokens (List[int]): A list of token IDs to decode.

        Returns:
            str: The decoded text.

        Raises:
            ValueError: If input is not a list of integers or if an invalid token ID is encountered.
        """
        if not isinstance(input_tokens, List):
            raise ValueError("Input must be a list of integers")

        bytes_array = bytearray()
        for token in input_tokens:
            if token in self.decode_dict:
                bytes_array.extend(self.decode_dict[token])
            else:
                raise ValueError(f"Invalid token id: {token}")

        decoded_text = bytes_array.decode("utf-8", errors="replace")
        return decoded_text

    def save(self, file_name: str, debug=False) -> None:
        """
        Save the trained tokenizer to a file.

        Args:
            file_name (str): The base name for the output file(s).
            debug (bool, optional): If True, also saves a human-readable
                version of the tokenizer. Defaults to False.

        Note:
            Saves the tokenizer to '{file_name}.bpe'.
            If debug is True, also saves a human-readable version of the
            tokenizer to '{file_name}_debug.bpe'.

        """

        output_file = file_name + ".bpe"

        with open(output_file, "w") as f:
            f.write(
                f"bytephase tokenizer by benjamin arnav v1\nregex patten: {self.pattern}\n"
            )
            for idx, token in self.decode_dict.items():
                token_ints = " ".join(str(b) for b in token)
                f.write(f"{idx} {token_ints}\n")

        if debug:
            # Outputs a human-readable version
            debug_filename = file_name + "_debug.bpe"
            with open(debug_filename, "w") as f:
                f.write(
                    f"bytephase tokenizer by benjamin arnav v1\nregex patten: {self.pattern}\n"
                )
                for idx, token in self.decode_dict.items():
                    token_chars = self.decode(list(token))
                    f.write(f"{idx} {token_chars}\n")

    def load(self, file: str) -> None:
        """
        Load a previously saved tokenizer from a .bpe file.

        Args:
            file (str): The path to the .bpe file to load.

        Raises:
            AssertionError: If the file format is invalid or incompatible.

        Note:
            This method updates the decode_dict attribute and rebuilds the C-based trie structure.
        """

        assert file.endswith(".bpe")

        with open(file, "r") as f:
            version_line = f.readline().strip()
            assert version_line == "bytephase tokenizer by benjamin arnav v1"
            regex_line = f.readline().split()
            assert regex_line[0] == "regex"

            for line in f:
                current_line = line.strip().split()
                idx = int(current_line[0])
                token = bytes(int(b) for b in current_line[1:])
                self.decode_dict[idx] = token

        self._trie = build_trie(self.decode_dict)

    def get_vocab_size(self) -> int:
        """
        Get the size of the vocabulary.

        Returns:
            int: The size of the vocabulary.
        """
        return len(self.decode_dict.keys())
