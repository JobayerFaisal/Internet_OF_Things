import logging
import asyncio
import aiocoap
logging.basicConfig(level=logging.DEBUG)
async def main():
    # Create a client context for CoAP communications.
    protocol = await aiocoap.Context.create_client_context()

    # Define the CoAP URI for the "light" endpoint.
    # Replace "localhost" with the actual server IP if needed.
    uri = "coap://192.168.0.107:5683/light"
    #uri = "coap://127.0.0.1/light"
    # Prepare the payload: "1" to, for instance, turn on the light.
    payload = "1".encode('utf-8')
    
    # Create a PUT request message with the payload.
    request = aiocoap.Message(code=aiocoap.PUT, uri=uri, payload=payload)

    try:
        # Send the request and wait for the response.
        response = await protocol.request(request).response
        print(response)
    except Exception as e:
        print("Failed to send PUT request:", e)
    else:
        print("Response Code:", response.code)
        print("Response Payload:", response.payload.decode('utf-8'))

if __name__ == "__main__":
    asyncio.run(main())
