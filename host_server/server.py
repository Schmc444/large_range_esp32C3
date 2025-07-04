from flask import Flask, request, jsonify
import json
import os
import sys
import signal
import argparse
from datetime import datetime
import logging

app = Flask(__name__)

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Global flag for graceful shutdown
shutdown_flag = False

def signal_handler(sig, frame):
    """Handle Ctrl+C gracefully"""
    global shutdown_flag
    print('\n🛑 Shutting down Solar Monitor Server gracefully...')
    print('📊 Server stopped. Log files preserved.')
    shutdown_flag = True
    sys.exit(0)

# Register signal handler for Ctrl+C
signal.signal(signal.SIGINT, signal_handler)

# Directory to store log files
LOG_DIR = "solar_logs"
if not os.path.exists(LOG_DIR):
    os.makedirs(LOG_DIR)

def get_log_filename():
    """Generate filename based on current date"""
    today = datetime.now().strftime("%Y-%m-%d")
    return os.path.join(LOG_DIR, f"solar_log_{today}.txt")

def format_log_entry(data):
    """Format the log entry for writing to file"""
    timestamp = datetime.fromtimestamp(data.get('timestamp', 0))
    formatted_time = timestamp.strftime("%H:%M:%S")
    
    entry = (
        f"{formatted_time} | "
        f"Device: {data.get('device_id', 'unknown')} | "
        f"WiFi RSSI: {data.get('wifi_rssi', 'N/A')} dBm | "
        f"Free Heap: {data.get('free_heap', 'N/A')} bytes | "
        f"Uptime: {data.get('uptime', 0) / 1000:.1f}s"
    )
    return entry

@app.route('/solar-log', methods=['POST'])
def receive_log():
    try:
        # Get JSON data from ESP32
        data = request.get_json()
        
        if not data:
            return jsonify({"error": "No data received"}), 400
        
        # Get current log filename
        log_file = get_log_filename()
        
        # Check if this is a new day (create header)
        file_exists = os.path.exists(log_file)
        
        # Format log entry
        log_entry = format_log_entry(data)
        
        # Write to file
        with open(log_file, 'a') as f:
            if not file_exists:
                # Write header for new day
                today = datetime.now().strftime("%Y-%m-%d")
                f.write(f"=== Solar Power Monitor Log - {today} ===\n")
                f.write("Time     | Device              | WiFi RSSI | Free Heap    | Uptime\n")
                f.write("-" * 80 + "\n")
            
            f.write(log_entry + "\n")
        
        logger.info(f"Log entry written: {log_entry}")
        
        return jsonify({
            "status": "success", 
            "message": "Log received and saved",
            "filename": os.path.basename(log_file)
        }), 200
        
    except Exception as e:
        logger.error(f"Error processing log: {str(e)}")
        return jsonify({"error": str(e)}), 500

@app.route('/solar-log/status', methods=['GET'])
def get_status():
    """Get current status and recent logs"""
    try:
        log_file = get_log_filename()
        
        status = {
            "current_date": datetime.now().strftime("%Y-%m-%d"),
            "log_file": os.path.basename(log_file) if os.path.exists(log_file) else "No log today",
            "file_exists": os.path.exists(log_file)
        }
        
        # Get last few lines if file exists
        if os.path.exists(log_file):
            with open(log_file, 'r') as f:
                lines = f.readlines()
                status["total_entries"] = len([l for l in lines if not l.startswith('=') and not l.startswith('-') and not l.startswith('Time')])
                status["last_entries"] = [line.strip() for line in lines[-5:] if line.strip()]
        
        return jsonify(status), 200
        
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route('/solar-log/files', methods=['GET'])
def list_log_files():
    """List all available log files"""
    try:
        files = []
        for filename in os.listdir(LOG_DIR):
            if filename.startswith('solar_log_') and filename.endswith('.txt'):
                filepath = os.path.join(LOG_DIR, filename)
                files.append({
                    "filename": filename,
                    "size": os.path.getsize(filepath),
                    "modified": datetime.fromtimestamp(os.path.getmtime(filepath)).strftime("%Y-%m-%d %H:%M:%S")
                })
        
        return jsonify({"files": files}), 200
        
    except Exception as e:
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Solar Power Monitor Server')
    parser.add_argument('--daemon', '-d', action='store_true', 
                       help='Run in background (daemon mode)')
    parser.add_argument('--port', '-p', type=int, default=5000,
                       help='Port to run server on (default: 5000)')
    parser.add_argument('--host', type=str, default='0.0.0.0',
                       help='Host to bind to (default: 0.0.0.0)')
    args = parser.parse_args()
    
    if args.daemon:
        # Run in daemon mode (background)
        print(f"🚀 Starting Solar Power Monitor Server in daemon mode...")
        print(f"📡 Server running on http://{args.host}:{args.port}")
        print(f"📁 Log files stored in: {os.path.abspath(LOG_DIR)}")
        print(f"🔍 Check status at: http://{args.host}:{args.port}/solar-log/status")
        print(f"⚡ To stop: pkill -f 'python.*server.py' or find the process and kill it")
        
        # Redirect stdout to a log file in daemon mode
        log_file = os.path.join(LOG_DIR, 'server.log')
        sys.stdout = open(log_file, 'a')
        sys.stderr = open(log_file, 'a')
        
        app.run(host=args.host, port=args.port, debug=False)
    else:
        # Run in interactive mode
        print("🌞   SOLAR POWER MONITOR SERVER")
        print(f"📡 Server starting on http://{args.host}:{args.port}")
        print(f"📁 Log files will be stored in: {os.path.abspath(LOG_DIR)}")
        print(f"🔍 Status endpoint: http://{args.host}:{args.port}/solar-log/status")
        print(f"📋 List files: http://{args.host}:{args.port}/solar-log/files")
        print("🛑 Press Ctrl+C to stop the server gracefully")
        
        app.run(host=args.host, port=args.port, debug=True)
