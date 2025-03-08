def read_and_write_text(input_file_path, output_file_path):
    try:
        # Open the input file for reading with UTF-8 encoding
        with open(input_file_path, 'r', encoding='utf-8') as input_file:
            with open(output_file_path, 'w') as output_file:
                for line in input_file:
                    state = 0
                    counter = 0
                    book = ''
                    chapter = ''
                    verse = ''
                    words = []
                    output = ''  # Initialize output variable
                    for char in line:
                        if state == 0:
                            if counter != 0 and char.isdigit():
                                book = output[:-1]
                                output = char
                                state += 1
                            else:
                                output += char
                        elif state == 1:
                            if char == ':':
                                chapter = output
                                output = ''
                                state += 1
                            else:
                                output += char
                        elif state == 2:
                            if not char.isdigit():
                                verse = output
                                output = ''
                                state += 1
                            else:
                                output += char
                        elif state == 3:
                            if not char.isalpha() and len(output) > 0:
                                words.append(output)
                                output = ''
                            elif char.isalpha():
                                output += char
                        counter += 1
                    # if the verse ended on a word
                    if output.isalpha():
                        words.append(output)
                    output_file.write(book+'\n')
                    output_file.write(chapter+'\n')
                    output_file.write(verse+'\n')
                    output_file.write('{\n')
                    for word in words:
                        output_file.write(word+'\n')
                    output_file.write('}\n')

    except FileNotFoundError:
        print(f"Error: File not found - {input_file_path}")
    except Exception as e:
        print(f"An error occurred: {e}")

# Example usage
input_file_path = 'kjv.txt'
output_file_path = 'kjvformatted.txt'

read_and_write_text(input_file_path, output_file_path)

