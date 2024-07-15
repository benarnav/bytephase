# bytephase
          
bytephase is a high-performance Byte Pair Encoding (BPE) tokenizer for the digital age that combines the power of Python with the speed of C extensions. Perfect for natural language processing tasks and training large language models.

I built this as part of a larger project to [implement GPT2](https://github.com/benarnav/gpt2) from scratch using only research papers. BPE was popularized by [Sennrich et al.](https://arxiv.org/abs/1508.07909) and further adapted to merge at the byte level in the [GPT2 paper](https://d4mucfpksywv.cloudfront.net/better-language-models/language_models_are_unsupervised_multitask_learners.pdf). This algorithm used to train some of the most popular LLMs.
   
### ✨ Features

- 🏎️ Fast tokenization with C extensions
- 🧠 Custom regex pattern support (defaults to the GPT-2 pattern)
- 🛠️ Train on your own data
- 💾 Save and load trained models
- 🔄 Encode and decode seamlessly
- 🐍 Pure Python implementation with C acceleration

### 🛠️ Installation
```bash
pip install git+https://github.com/benarnav/bytephase.git
```

### ⚡️ Quick Start
```python
from bytephase import Tokenizer

# Initialize and train
tokenizer = Tokenizer()
tokenizer.train("path/to/your_data.txt", vocab_size=50257)

# Encode
encoded = tokenizer.encode("Hello, world!")
# [11867, 44, 1561, 33, 256]

# Decode
decoded = tokenizer.decode(encoded)
# "Hello, world!"

# Save and load
tokenizer.save("saved_tokenizer")
tokenizer.load("saved_tokenizer.json")
```

### ⚙️ Encoding Performance
Tested on [first 10k elements](https://huggingface.co/datasets/NeelNanda/pile-10k) of [The Pile](https://huggingface.co/datasets/EleutherAI/pile) (50 runs each mode):
| Mode      | Speed (tokens/s) | Memory Usage (MB) |
|-----------|------------------:|------------------:|
| Train     |      1.42M |            735 |
| Inference |      1.82M |          18,676 |

These tests each used a PyTorch dataloader to loop over the text, and are therefore directly comparable. However, a more realistic test for inference would be to load the entire text into memory and then encode it. In this scenario, encoding is significantly faster:
| Mode      | Speed (tokens/s) | Memory Usage (MB) |
|-----------|------------------:|------------------:|
| Inference |      2.41M |          19,220 |

#### Usage
```python
# Set the train_mode flag when encoding, defaults to True

# Encode using train mode (default)
encoded = tokenizer.encode("Hello, world!", train_mode=True)

# Encode using inference mode
encoded = tokenizer.encode("Hello, world!", train_mode=False)
```


### How It Works
bytephase implements a Byte Pair Encoding algorithm with the following key components:

1. Customizable regex pattern for initial tokenization
2. Efficient training process using C extensions
3. Fast encoding using a trie data structure (implemented in C), with two modes for training and inference.
4. Seamless decoding of token IDs back to text

### 🔬 Advanced Usage
#### Custom Regex Pattern
```python
custom_pattern = r'\w+|\s+|[^\w\s]+'
tokenizer = Tokenizer(pattern=custom_pattern)
```
#### Custom File Read Buffer
The `Tokenizer` class allows you to specify a custom file read buffer size (in bytes) when initializing. This can be useful when working with large files or optimizing for specific system configurations. Default is 2MB.
```python
# Create a tokenizer with a 4MB buffer
tokenizer = Tokenizer(file_read_buffer=4194304)
```
#### Encode with model sequence length
Specify a sequence length and the `encode` method will pad the output if input is less than length, or clip the output if it's longer.
```python
tokenizer.encode("Hello, world!", seq_len=10)
# [11867, 44, 1561, 33, 256, 256, 256, 256, 256, 256]
# where 256 is the end of sequence token
```
#### Debug Mode
This will generate an additional human-readable file for easier inspection of the trained tokenizer.
```python
tokenizer.save("saved_tokenizer", debug=True)
```

### 🔮 Future Plans
- Add more special tokens used in models like GPT4
- Add ability to load other training files, like those for GPT4

### 🤝 Contributing
Contributions are welcome! Please post an issue first describing the issue before a pull request.
