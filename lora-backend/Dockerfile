# -------- Stage 1: Build --------
FROM gradle:8.14-jdk17-alpine AS build
WORKDIR /app

# Authors
LABEL authors="Mehmet Furkan Kaya <mefurkankaya36@gmail.com>"

# Cache dependencies first
COPY build.gradle settings.gradle ./
COPY gradle/ gradle/
RUN gradle dependencies --no-daemon

# Copy source and build
COPY src ./src
RUN gradle clean build -x test --no-daemon

# -------- Stage 2: Runtime --------
# Use a slim JRE, not full JDK
FROM eclipse-temurin:17-jre-alpine
WORKDIR /app

# Copy only the JAR from the build stage
COPY --from=build /app/build/libs/*.jar app.jar

ENTRYPOINT ["java", "-jar", "app.jar"]