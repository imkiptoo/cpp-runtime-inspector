#include <iostream>
#include <string>
#include <cstdint>

struct City
{
public:
  City(std::string country, size_t population, uint8_t rivers);

private:
  std::string m_country;
  size_t m_population;
  uint8_t m_rivers;
};

City::City(std::string country, size_t population, uint8_t rivers)
  : m_country(country), m_population(population), m_rivers(rivers)
{
};

int add(int x, int y)
{
  int z = x + y;
  std::cout << z << std::endl;

  for(int i = 0; i < 10000; i++)
  {
    z++;
  }

  return z;
}

int main() {
  City* stuttgart = new City("Germany", 1000000, 2);

  int* x = new int(0x50);
  auto z = new uint8_t(4);
  int y = 10;
  int sum = add(*x, y);

  std::cout << "Sum: " << sum << std::endl;

  delete(x);

  return 0;
}
