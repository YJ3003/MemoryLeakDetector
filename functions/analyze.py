import json

def handler(event, context):
    # Extract the code sent from the front-end
    body = json.loads(event['body'])
    code = body.get('code', '')

    # Process the C code (add your memory leak detection logic here)
    result = f"Processed code: {code}"

    # Return a JSON response
    return {
        'statusCode': 200,
        'body': json.dumps({'result': result})
    }
