import os
import subprocess
from flask import Flask, render_template, request, jsonify
import tempfile

app = Flask(__name__)

# Path to store the temporary C file and the compiled executable
TEMP_DIR = tempfile.mkdtemp()

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/analyze', methods=['POST'])
def analyze_code():
    try:
        # Receive C code from the frontend
        code = request.json.get('code')

        # Save the received C code to a temporary C file
        c_file_path = os.path.join(TEMP_DIR, 'temp_code.c')
        with open(c_file_path, 'w') as f:
            f.write(code)

        # Include the leak_detector.h file in the C code for memory leak detection
        leak_header = open('leak_detector.h', 'r').read()
        with open(c_file_path, 'r+') as f:
            original_code = f.read()
            f.seek(0, 0)
            f.write(leak_header + '\n' + original_code)

        # Compile the C code
        compile_command = ['gcc', c_file_path, '-o', os.path.join(TEMP_DIR, 'temp_program'), '-Wall']
        compile_process = subprocess.run(compile_command, capture_output=True, text=True)

        if compile_process.returncode != 0:
            # Compilation failed, return error message
            return jsonify({'error': compile_process.stderr}), 400

        # Run the compiled program to get the memory leak report
        run_command = os.path.join(TEMP_DIR, 'temp_program')
        run_process = subprocess.run(run_command, capture_output=True, text=True)

        if run_process.returncode != 0:
            # Running the program failed
            return jsonify({'error': run_process.stderr}), 400

        # Return the output (memory leak report) as a response
        return jsonify({'result': run_process.stdout})

    except Exception as e:
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    app.run(debug=True)
