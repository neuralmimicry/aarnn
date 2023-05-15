# Use an official C++ runtime as a parent image
FROM ubuntu:latest

# Set the working directory in the container to /app
WORKDIR /app

# Copy the compiled executable from your host to the current location in your docker image
COPY ./build/AARNN /app/AARNN

# Install necessary libraries for running the application
RUN apt-get update && apt-get install -y libpqxx-dev

# Make port 80 available to the world outside this container (optional)
EXPOSE 80

# Run the application when the container launches
CMD ["./AARNN"]
